# Data link protocol

## Get started
1. Compile the application and the virtual cable program using the provided Makefile.
2. Run the virtual cable program (either by running the executable manually or using the makefile target):
	`$ ./cable_app`
	`$ make run_cable`
3. Test the protocol
	3.1 Run the receiver (either by running the executable manually or using the Makefile target):
		`$ ./application_main /dev/ttyS11 rx penguin-received.gif`
		`$ make run_tx`
	3.2 Run the transmitter (either by running the executable manually or using the Makefile target):
		`$ ./application_main /dev/ttyS10 tx penguin.gif`
		`$ make run_rx`
	3.3 Should have received a nice looking penguin. You can check if both files are the same using the diff Linux command, or using the Makefile target:
		`$ diff -s penguin.gif penguin-received.gif`
		`$ make check_files`
4. Test the protocol with cable unplugging and noise
	4.1. Run receiver and transmitter again
	4.2. Quickly move to the cable program console and press 0 for unplugging the cable, 2 to add noise, and 1 to normal
	4.3. Should have received a nice looking penguin even if the cable disconnected or with noise added
