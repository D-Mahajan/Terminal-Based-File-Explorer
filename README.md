# Terminal File Explorer
* Location of code file is current working directory and `home` for file explorer.
* Program can be exited using `Cntl+x`.

## Normal Mode
* Default mode of file explorer, allowing user to navigate through file system using keyboard shortcuts.
* Displays directory content similar to `ls` command output in Unix based systems. 

### Keyboard Shortcuts
1. Up: Up arrow key
2. Down: Down arrow key
3. Scroll Up: `k`
4. Scroll Down: `l`
5. Open Directory/File: `Enter`
6. Previously visited directory: Left arrow key
7. Next directory: Right arrow key
8. Go up one level: Backspace
9. Home: `h`

## Command Mode
* Allows user to enter different commands at the bottom of the screen.

### Commands
1. Copy: `copy <source_file(s)/directory(ies)> <destination_directory>`
2. Move: `move <source_file(s)/directory(ies)> <destination_directory>`
3. Rename: `rename <old_filename> <new_filename>`
4. Create File: `create_file <file_name> <destination_path>` <br>
    Destination path should be relative to home.
5. Create Directory: `create_dir <dir_name> <destination_path>` <br>
    Destination path should be relative to home.    
6. Delete File: `delete_file <file_path>`<br>
    Destination path should be relative to home.    
7. Delete Directory: `delete_dir <dir_path>`<br>
    Destination path should be relative to home.
8. Goto: `goto <location>` <br>
    Location is absolute path w.r.t. home.
9. Search: `search <file_name>` or `search <directory_name>`
10. Exit command mode: `Esc` key