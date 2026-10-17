 On the ps2 branch within the OS-Doom repo, you can use either the run_qemu_withGUI or 
 the run_qemu_noGUI executables, though you will need to do X11 forwarding for the former 
 to work. There is also a flag called DEMO within ps2.cc that you can set to true for the 
 keyboard input to be printed cleanly,or you can set the flag to false for debug information 
 to print. If running with no GUI, simply type commands like "scankey a", "mouse_move 100 100", 
 or "mouse_button 1" to inject input via the QEMU terminal. If running with GUI, simply focus 
 the QEMU window and type on your keyboard or move/click with your mouse.

There's also a hangman testcase which checks whether user inputs and interrupts are being 
handled correctly that can be called by running either executable on the "hangman" testcase 
(DEMO flag must be true for the testcase to work).

For the poll testcase, the DEMO flag in ps2.cc needs to be set to false to function, in the 
first few tests, you will be required to provide input keyboard input and mouse input. Do 
note that the final test is meant to be an infinite loop, therefore the test will never shutdown 
on its own, you will need to shut it down manually with Ctrl + C in the terminal. Also, the poll 
testcase is found in testcases/input-interface/t1.