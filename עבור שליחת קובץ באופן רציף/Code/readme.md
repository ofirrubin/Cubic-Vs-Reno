To run the test you gotta make sure the variables are set:
Please create a file named file.txt either by file_gen.py or by you.

Please run all commands described in this folder.

To use file_gen.py you can run the follow command:

"python3 file_gen.py <size>" where <size> is the file size in bytes.

Then, you'll have to change the Measure, sender settings by variables defined in the top of each script.
Set the filename, test size (how many times to run each test), server addr, port etc. in sender script.
Set the server address, port, test size, and default recieved file size in the Measure script. 
Default recieved size is the file.txt size that can be obtained through the file properties. It's a safe machenisem if the way I'm working with 
doesn't work (if it gets file size 0 etc.) it will use this size (I had times it used this).

Then use the makefile to create the required files:

"make all"

After each run you can remove the generated files using "make clean".

Then you can run the test programs,

Run the server by the command "./Measure"

Then run the client by the command "./sender" with another terminal.

The result will be printed in "./Measure" stdout.

