#if file_exists(relative_path.inc)
    #include relative_path.inc
#else
    relative_to_wrong_directory_1: { }
#endif

#if file_exists(present.inc)
    relative_to_wrong_directory_2: { }
#endif