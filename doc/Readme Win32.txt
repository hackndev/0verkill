0verkill 0.15pre3 Win32 version - User's Guide
==============================================
To compile 0verkill, you must have Microsoft Visual C++ 6.0 (you might be able to compile with previous versions as well, but you would have to create your own projects - DSPs - according to the ones contained in this distribution).

* Open the workspace (0verkill.dsw).
* From the Build menu, select Batch build.
* Enable the appropriate configurations (0verkill Release, Server Release, Bot Release).
* Choose Build.
Now, the binaries should be generated. You may notice many warnings, but these should be ok. If you experience any errors, you may try to fix it yourself, or contact me (see below).
* Now, the binaries should be in the 0verkill\Release and Server\Release subdirectories. Move them to the "root" directory (i.e. the one containing 0verkill.dsw).

To run the server in console mode, type "server -c".

To install the server as a Windows NT service (on Windows 2000 or Windows NT 3.51+), follow these steps:
* log on as an administrator
* if you want, you may create a special user account for the service. The account must have the appropriate privilegies (log on as service, particularly).
* type "server -I" to install the service. This will cause the service to use the LOCAL_SYSTEM account. To use another account, type "server -i account/password". Note that for local accounts, you must add the ".\" prefix to the account name to specify that it is the LOCAL account. For example, if you have created an account 0verkill with password 0verkill at the server machine, you would use "server -i .\0verkill/0verkill".
* start the service by typing "net start 0verkill_015". To stop the service, type "net stop 0verkill_015". You may also configure the service to start automatically at system startup. In Windows NT, go to Control Panel/Services/0verkill_015. In Windows 2k, go to Control Panel/Admin.Tools/Services.
To stop and uninstall the server, type "server -r".

Remember that the server.exe file must be in the game root directory. If you need to put it elsewhere, specify command line arguments:
(console mode:)
"server c:\0verkill"
(note that you must not use the "-c" flag)
You can also specify a special game root directory in the Control Panel for the service version by typing the desired directory as the "(command line) arguments" in the service properties window.

To run the client, type "0verkill". You can create a shortcut to 0verkill.exe and customize the window size. You can adjust the window size during gameplay as well.

Hope you'll enjoy the Win32 port!

Filip Konvicka, author of the port
mailto:filip.konvicka@seznam.cz
