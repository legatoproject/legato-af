TLA model of Legato copy-from-golden process

Copyright (C) Sierra Wireless Inc.

To check the specification:

1) Translate PlusCalc algorithm description.
2) Run the model.  Should have:
    * Temporal formula: RebootSpec
    * Check invariants: Invariants
    * Check properties: <>Running
    * Do NOT check deadlock: TLA+ will always report a deadlock as
      disk flush process does not terminate.

Before checking in, remove the TLA+ translation of the algorithm (code
between BEGIN_TRANSLATION and END_TRANSLATION).

------------------------------- MODULE start -------------------------------

EXTENDS Sequences, TLC, Integers

(* Use special IDs for the "unpack" and "current" system. *)
UNPACK_SYSTEM == -501
CURRENT_SYSTEM == -500

(***** Key types *****)
(* Modeled system indexes *)
SystemIndexes == { 0, 1, 2, 3, 4, 5 } \* Only 0 and 1 should be required for now
(* Allowed system states:
 * none - nothing present in this system
 * mixed - some files are present, but not all.
 * good - all files have been copied, symlinks set up, etc. *)
System_State == { "good", "mixed", "none" }
(* Legato uses a couple named systems: "current" and "unpack".
 Model these using otherwise invalid system IDs.  Using all
 numbers simplfies the model *)
Named_Systems == { UNPACK_SYSTEM, CURRENT_SYSTEM }
(* States recorded in the ld.so.cache file *)
LdSoCache_State == { "needed", "started", "none" }
(* Valid systems -- can be numbered or named *)
Systems == Named_Systems \union SystemIndexes

(***************************************************************************

--algorithm Start {
    (* State of the system -- both in memory and on disk.
       In this description its assumed that the Linux disk write functions
       write to a cache, and at some point later that cache is flushed to
       disk.

       Cached state:
       system_state: State of each system
       index: Index recorded for each system
       ldcache_state: Contents of the ldcache file (if it exists).
       golden_installed: Whether the golden image has been marked as installed.

       disk_*: State recorded on disk.

       current_index, newest_index: indexes calculated when deciding
           what system to boot

       has_rebooted: If the system has rebooted.  This model assumes only
          a single failure.
     *)
    variable systems_state = [ system \in Systems |->  "none" ],
             index = [ system \in Systems |-> -1 ],
             ldcache_state = "none",
             golden_installed = FALSE,
             disk_systems_state = [ system \in Systems |-> "none" ],
             disk_index = [ system \in Systems |-> -1 ],
             disk_ldcache_state = "none" ,
             disk_golden_installed = FALSE,
             current_index = -1,
             newest_index = -1,
             has_rebooted = FALSE

    define
    {
        (* The set of systems on disk *)
        ActiveSystems == { system \in Systems : systems_state[system] # "none" }
        (* Most recent system, or -1 of no systems are present on disk *)
        NewestIndex == IF ActiveSystems = {}
                       THEN -1
                       ELSE LET newest == CHOOSE system \in ActiveSystems :
                               ( \forall other_system \in ActiveSystems :
                                    index[system] >= index[other_system] )
                            IN index[newest]
        (* Currently active system, or -1 if no current system *)
        CurrentIndex == IF CURRENT_SYSTEM \in ActiveSystems
                        THEN index[CURRENT_SYSTEM]
                        ELSE -1
        (* Systems (or indexes) which have different contents in cache than on disk *)
        DirtySystems == { system \in Systems : systems_state[system] # disk_systems_state[system] }
        DirtyIndexes == { system \in Systems : index[system] # disk_index[system] }

        (* Has the state been flushed to disk?  "await Sync" models calling sync() on Linux *)
        Sync == /\ disk_systems_state = systems_state
                /\ disk_index = index
                /\ disk_ldcache_state = ldcache_state
                /\ disk_golden_installed = golden_installed
    }

    (* Models calling RecursiveDelete() on a system:
     * First the files are deleted recursively (state becomes mixed),
     * then the directory is deleted (state becomes none). *)
    procedure DeleteSystem(delete_system = -1)
    {
        delete1: systems_state := [  systems_state EXCEPT ![delete_system] = "mixed" ];
        delete2: systems_state := [  systems_state EXCEPT ![delete_system] = "none" ];
        return;
    }

    (* Models calling Rename() on a system. *)
    procedure RenameSystem(original = -1, new = -1)
    {
        rename:
        index[new] := index[original] ||
        index[original] := -1;
        systems_state[new] := systems_state[original] ||
        systems_state[original] := "none";
        disk_index[new] := disk_index[original] ||
        disk_index[original] := -1;
        disk_systems_state[new] := disk_systems_state[original] ||
        disk_systems_state[original] := "none";
        return;
    }

    (* Models InstallGolden() function *)
    procedure InstallGolden()
    variable golden_index = newest_index + 1;
    {
        delete_old: call DeleteSystem(golden_index);
        rename_current1: if (current_index > -1)
        {
             rename_current2:
             call RenameSystem(CURRENT_SYSTEM, current_index);
        };

        unpack1: systems_state[UNPACK_SYSTEM] := "mixed";
        unpack2: index[UNPACK_SYSTEM] := golden_index;
        unpack3: systems_state[UNPACK_SYSTEM] := "good";
        rename_unpack: call RenameSystem(UNPACK_SYSTEM, CURRENT_SYSTEM);
        delete_all_but_current:
        while ( ActiveSystems \ { CURRENT_SYSTEM } # {} )
        {
            delete_one:
            call DeleteSystem(CHOOSE system \in ActiveSystems : system # CURRENT_SYSTEM);
        };

        request_cache:
        ldcache_state := "needed";

        flush_system:
        await Sync;

        mark_install_complete:
        golden_installed := TRUE;
        newest_index := golden_index;
        current_index := golden_index;
        return;
    }

    (* Models CheckAndInstallCurrentSystem *)
    fair process ( InstallGoldenProc = 0 )
    {
        DeleteSystemUnpack: call DeleteSystem(UNPACK_SYSTEM);

        GetIndexes: newest_index := NewestIndex;
        current_index := CurrentIndex;

        ShouldInstallGolden: if (newest_index = -1 \/ ~golden_installed)
        {
            DoInstallGolden: call InstallGolden();
        };

        NeedLdConfig:
        if (ldcache_state # "none")
        {
            start_ldcache: ldcache_state := "started";
            done_ldcache: ldcache_state := "none";
        };
    }

    (* Models Linux disk caching.
     * If anything data is out of sync between in-memory cache and disk it can
     * be written to disk *)
    fair process ( DiskFlushProc = 1 )
    {
        flush_data:
        while (TRUE)
        {
            either
            {
                with (index_sync \in DirtyIndexes)
                {
                    disk_index[index_sync] := index[index_sync];
                };
            }
            or
            {
                with (system_sync \in DirtySystems)
                {
                    disk_systems_state[system_sync] := systems_state[system_sync];
                }
            }
            or
            {
                await disk_ldcache_state # ldcache_state;
                disk_ldcache_state := ldcache_state;
            }
            or
            {
                await disk_golden_installed # golden_installed;
                disk_golden_installed := golden_installed;
            }
        };
    }
}

****************************************************************************)

(* Translation of above algorithm into TLA+.  Done automaticall -- this
 * is needed to run model verification. *)
\* BEGIN TRANSLATION
\* END TRANSLATION

(* Model the reboot process.

   Can only occur if the board has not previously rebooted (only a single
   failure is modeled.  Resets system state back to beginning, except:

   * on-disk state is preserved,
   * cached state is loaded from disk state,
   * the system is marked as rebooted.
   *)

Reboot == /\ ~has_rebooted
          /\ (pc' = 0 :> "DeleteSystemUnpack" @@ 1:> "flush_data")
          /\ index' = disk_index
          /\ systems_state' = disk_systems_state
          /\ ldcache_state' = disk_ldcache_state
          /\ golden_installed' = disk_golden_installed
          /\ current_index' = -1
          /\ newest_index' = -1
          /\ has_rebooted' = TRUE
          (* Procedure DeleteSystem *)
          /\ delete_system' = [ self \in ProcSet |-> -1]
          (* Procedure RenameSystem *)
          /\ original' = [ self \in ProcSet |-> -1]
          /\ new' = [ self \in ProcSet |-> -1]
          (* Procedure InstallGolden *)
          /\ golden_index' = [ self \in ProcSet |-> newest_index + 1]
          (* Process DiskFlushProc *)
          /\ stack' = [self \in ProcSet |-> << >>]
          /\ UNCHANGED << disk_systems_state, disk_index,
                          disk_ldcache_state, disk_golden_installed >>

(* Spec for this system is the spec for the basic algorithm, but reboots
   are also allowed.  *)
NextReboot == Next \/ Reboot

RebootSpec == /\ Init /\ [][NextReboot]_vars
        /\ /\ WF_vars(InstallGoldenProc)
           /\ WF_vars(DeleteSystem(0))
           /\ WF_vars(InstallGolden(0))
           /\ WF_vars(RenameSystem(0))
        /\ WF_vars(DiskFlushProc)
        /\ WF_vars((pc[0] # "Done") /\ Reboot)

(* Is the system running?  Model has temporal property that for all states, eventually
   the system will be running *)
Running == /\ pc[0] = "Done"
           /\ systems_state[CURRENT_SYSTEM] = "good"
           /\ golden_installed
           /\ ldcache_state = "none"

(* Invariant properties of the system:
   1) Should only be marked as golden_installed if current system state is good.
   This will also be caught by above, but adding here catches issues sooner *)

Invariants == /\ golden_installed => systems_state[CURRENT_SYSTEM] = "good"
              /\ disk_golden_installed => disk_systems_state[CURRENT_SYSTEM] = "good"

=============================================================================
\* Modification History
\* Last modified Tue May 23 16:31:10 PDT 2017 by kdunwoody
\* Created Mon May 15 11:04:45 PDT 2017 by kdunwoody
