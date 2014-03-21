Migration
=========

#### Location

The migration logic itself will be put in application code, not in
Agent. The Agent only provides a generic module that can
automatically load migration scripts.

#### Migration process

The migration module will determine if a migration is needed.\
 For that purpose, it will use a persisted object named
**"AgentVersion"**.\
 The content of this persisted object is the value of
**MIHINI_AGENT_RELEASE** Lua global variable (the version is also
displayed in the logs).

If the content of persisted object is different of current version
result then the migration will be done.\
 This includes the case where the persisted object doesn't exist, then
versionFrom migration param (see bellow) will be set to "unknown".

Then the user/project specific migration code is done.\
 Finally, the persisted object value is set with current version.

#### User/Project specific code

Lua API only.\
 When migration is needed, generic code will load user/project specific
migration code using:

~~~~{.lua}
require "agent.migration"
~~~~

It's up to the user/project to provide the migration code itself.
(Agent dev team is here to help of course.)\
 The migration code is very likely to be a Lua file named
"agent/migration.lua".\
 However, if the user migration needs to run C code, it is possible,
providing the C code exports a function:

~~~~{.lua}
int luaopen_agent_migration(LuaState* L);
~~~~

So that it can be loaded the same way if it was Lua file.

In any case, *agent.migration* **must** provide one function:

~~~~{.lua}
function execute(versionFrom, versionTo)
~~~~

This is the function that will be called in order to actually do the
migration, with those parameters:\
 versionFrom: string, current value of persisted of object, or "unknown"
if no Agent version was persisted yet.\
 versionTo: string, current value of getversion() result

> **WARNING**
>
> Please pay attention to make the migration script as light as possible,
> with minimum dependencies with others modules.\
>  agent.migration will be executed very early at the Agent boot
> phase, no configuration will be loaded, no module will be running, only
> low level features of Lua VM will be available.\
>  For example, do not use log module, when agent.migration will be
> executed, log is not configured yet.\
>  If you need to report some error, check next section: Migration status
> reporting

> **INFO**
>
> It is also the responsibility of the user/project to put the migration
> code in the application binary.\
>  By default, Agent only embeds a simple Lua template of migration code.

#### Migration status reporting

The user migration script needs to **throw Lua error** to report some
error in migration.\
 Otherwise, the migration script execution is considered as successful.\
 Note: if migration script is absent, an error will be reported once.

When migration script is triggered, migration execution status is logged
in Agent boot logs:

~~~~{.lua}
+LUALOG: "GENERAL","INFO","Migration executed successfully"
~~~~

#### Migration script templates

For Lua migration script template, pick up migration.lua\
 For C-Lua migration script template, pick up migration.c **and**
migration.h\
 CMakeLists.txt is an example of how to compile migration.c using CMake (the default build system used by the agent).


-----------------------------------------------------------------------------------------------------------------------------------------------------------
Name                                                                                                       Comment
------------------------------------------------------                                                     ------------------------------------------------
[migration.lua](http://git.eclipse.org/c/mihini/org.eclipse.mihini.git/tree/agent/agent/migration.lua)      Lua migration script template
                                                                                                           
[MigrationScript.h](Migration_MigrationScript_h.html)                                                       C-Lua migration script template header
                                                                                                           
[MigrationScript.c](Migration_MigrationScript_c.html)                                                       C-Lua migration script template module
                                                                                                           
[CMakeLists.txt](Migration_CMakeLists_txt.html)                                                             How to compile MigrationScript.c using CMake
-----------------------------------------------------------------------------------------------------------------------------------------------------------

