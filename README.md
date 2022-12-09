# GuardCode
GuardCode gives you the ability to monitor your application and catch rouge events.

## Usage
The usage of the tool is when you want to check a library\module does not have malicious intention.
To use one of the releases you will need to do those steps:
1. Extract files from Release.rar.
2. Get the driver (microsoft's or openprocmon's) and GuardCode executable in the same folder.\
example of doing the steps for the application with microsoft's driver:
![example for running application with microsoft's driver](https://github.com/noam242/GuardCode/blob/main/GIFs/extraction.gif)
3. Run cmd in administrator mode and change its directory to the GuardCode's folder.
4. Use the tool!!☺
### Fast usage
For "fast usage" you should run the tool before you install the library\module with this command:
```
GuardCode.exe -e <your_executable_name> -f Format.gcf
```
Then run your code and execute all of it's abilities.\
This will create a rule file with all the occured events in GuardCode Format and in whitelist, that means that the all of rules allow events but the last one "drops" all other events.\
Afterwards you can install the library\module and catch all of the library\module events maybe event stop them.\
If you trust the library\module a little bit you can use the tool with 'w' mode like so:
```
GuardCode.exe -e <your_executable_name> -m w -fi rules.gcf
```
This will alert you of events that deviated from the rules.\
If you do not trust the library\module you can use the tool with 'a' mode like so:
```
GuardCode.exe -e <your_executable_name> -m a -fi rules.gcf
```
This will stop your application if the events deviated from the rules.\
example of running the application with microsoft's driver:
![example for running application with microsoft's driver](https://github.com/noam242/GuardCode/blob/main/GIFs/cmdadminandrun.gif)

### Examples of usage:
If you want to only monitor your application:
```
GuardCode.exe -e example.exe
```
If you want that the tool will write rules for your application:
```
GuardCode.exe -e <your_executable_name> -f Format.gcf
```
If you want to check your rules for your application:
```
GuardCode.exe -e <your_executable_name> -m w -fi Format.gcf
```
If you want to activly protect your computer with your rules from your application:
```
GuardCode.exe -e <your_executable_name> -m a -fi Format.gcf
```
**sidenotes:**
* All of those examples by default write into a log file named "log.gcf".
* If you want to change the log file just add the flag:
```
-fo <name_of_log_file>.gcf
```
* If you want to see the logs file and the rules file in a user friendly way.\
download [GuardCodeFormat](https://github.com/noam242/GuardCode/blob/main/GuardCodeFormat.xml) and [notepad++](https://notepad-plus-plus.org/downloads/) and add it like so:
    1. Open notepad++ and go to "Language" tab.
    2. Go to "User Defined Language", then "Define your language".
    3. Click on import and find the path of "GuardCodeFormat.xml".
    4. When imported successfully you should see it in one of the "User language:" tabs, Then restart the notepad++ application.
    5. When needed click on "Language" tab, then on GuardCodeFormat and you should see the colors.
![example for loading the format to notepad++](https://github.com/noam242/GuardCode/blob/main/GIFs/GuardCodeFormatNotepadLoading.gif)
## Expert usage

### Rules
Pointers of How to write the rules:
* one
* two






















## inspirations
The project that made me to start this project [hagana](https://github.com/yaakov123/hagana).\
The project that made my life much easier [openprocmon](https://github.com/progmboy/openprocmon).
