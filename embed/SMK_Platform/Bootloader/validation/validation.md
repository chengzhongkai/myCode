@mainpage Bootloader Validation Framework
Introduction
============

The Bootloader Framework is a set of C++ classes designed to run on a desktop PC.
The framework implements two major functions:

1. Bootloader Simulation - The framework classes implement a simulation of the bootloader running on the PC.
The simulation can execute bootloader commands and contains Flash and RAM memory models to validate the simulated
bootloader state before and after running a command. The simulation uses actual bootloader target code compiled for the PC.
2. Host Operations - The framework classes can format a bootloader command stream and send it to an actual target device
running the bootloader. In other words, these classes provide the low-level support for creating a host "firmware download"
application.

Block Diagram
=============

![Image not found](../image/blfw.png)

Simulator Operations
====================

The basic flow for the simulator is as follows:

	// Create a command
	Command * cmd = Command::create(&m_cmdv);

	// Create simulated host and device peripherals
	uchar_deque_t commandStream;
	uchar_deque_t responseStream;
	SimPeripheral hostPeripheral;
	SimPeripheral devicePeripheral;
	hostPeripheral.init(&responseStream, &commandStream);
	devicePeripheral.init(&commandStream, &responseStream);

	// Create simulated host and device packetizers
	SimPacketizer hostPacketizer;
	hostPacketizer.init(&hostPeripheral);
	SimPacketizer devicePacketizer;
	devicePacketizer.init(&devicePeripheral);

	// Enable the host packetizer to pump the simulator state machine as necessary.
	hostPacketizer.enableSimulatorPump();

	// Create the bootloader
	Bootloader * bl = Bootloader::getBootloader();
	bl->init(&hostPacketizer, &devicePacketizer);
	bl->openStateFiles(m_stateDir, m_initState);

	// Run the command in the simulator
	bl->inject(*cmd);

	// Clean up
	bl->flush();

Host Operations
===============

The basic flow for host operations is as follows:

	// Create a command
	Command * cmd = Command::create(&m_cmdv);

	// Create a peripheral to talk to the target, in this case PC UART
	UartPeripheral pcUartPeripheral;
	pcUartPeripheral.init(m_comPort, m_comSpeed);

	// Create the peripheral packetizer
	SerialPacketizer hostPacketizer;
	hostPacketizer.init(hostPeripheral);

	// Create the bootloader
	Bootloader * bl = Bootloader::getBootloader();
	bl->initWithHost(&hostPacketizer);

	// Send the command to the target
	bl->inject(*cmd);

	// Clean up
	bl->flush();

BLSIM Usage
===========

BLSIM is an executable that runs on the PC and runs a command through the bootloader simulator.

	usage: blsim [-?|--help] [-v|--version] [-s|--state <dir>] [-i|--init]
				 [-V|--verbose] [-d|--debug] [-j|--json] -- command <args...>

	Options:
	  -?/--help                    Show this help
	  -v/--version                 Display tool version
	  -i/--init                    Recreate state files
	  -s/--state <dir>             Put state files here
	  -V/--verbose                 Print extra detailed log information
	  -d/--debug                   Print really detailed log information
	  -j/--json                    Print output in JSON format to aid automation.

	Command:
	  reset                        Reset the chip
	  get-property <tag>
		1                          Bootloader version
		2                          Available peripherals
		3                          Start of program flash
		4                          Size of program flash
		5                          Size of flash sector
		6                          Blocks in flash array
		7                          Available commands
		8                          CRC check status
		10                         Verify Writes flag
		11                         Max supported packet size
		12                         Reserved regions
		13                         Validate regions flag
		14                         Start of RAM
		15                         Size of RAM
		16                         System device identification
		17                         Flash security state
	  set-property <tag> <value>
		10                         Verify Writes flag
		13                         Validate regions flag
	  flash-erase-region <addr> <byte_count>
								   Erase a region of flash
	  flash-erase-all              Erase all flash, excluding protected regions
	  flash-erase-all-unsecure     Erase all flash, including protected regions
	  read-memory <addr> <byte_count> <file>
								   Read memory from file
	  write-memory <addr> [<file> | {{<hex-data>}}]
								   Write memory from file or string of hex values,
								   e.g. "{{11 22 33 44}}"
	  fill-memory <addr> <byte_count> <pattern> [word | short | byte]
								   Fill memory with pattern; size is
									 word (default), short or byte
	  receive-sb-file <file>       Receive SB file
	  execute <addr> <arg> <stackpointer>
								   Execute at address with arg and stack pointer
	  call <addr> <arg>            Call address with arg
	  flash-security-disable <key> Flash Security Disable <8-byte-hex-key>,
									 e.g. 0102030405060708

BLHOST Usage
============

BLHOST is an application that runs on the PC and sends a command to target hardware connected over UART or another supported peripheral.

	usage: blhost [-?|--help] [-v|--version] [-p|--port <name>[,<speed>]]
				  [-b|--buspal spi[,<speed>,<polarity>,<phase>,lsb|msb] | i2c,[<address>,<speed>]]
				  [-u|--usb [[[<vid>,]<pid>]]] [-V|--verbose] [-d|--debug]
				  [-j|--json] [-n|--noping] [-t|--timeout <ms>]
				  -- command <args...>

	Options:
	  -?/--help                    Show this help
	  -v/--version                 Display tool version
	  -p/--port <name>[,<speed>]   Connect to target over UART. Specify COM port
								   and optionally baud rate
									 (default=COM1,57600)
									 If -b, then port is BusPal port
	  -b/--buspal spi[,<speed>,<polarity>,<phase>,lsb|msb] |
				  i2c[,<address>,<speed>]
								   Use SPI or I2C for BusPal<-->Target link
								   All parameters between square brackets are
								   optional, but preceding parameters must be
								   present or marked with a comma.
								   (ex. -b spi,1000,0,1) (ex. -b spi,1000,,lsb)
									 spi: speed(KHz),
										  polarity(0=active_high | 1=active_low),
										  phase(0=rising_edge | 1=falling_edge),
										  "lsb" | "msb"
										  (default=100,1,1,msb)
									 i2c: address(7-bit hex), speed(KHz)
										  (default=0x10,100)
	  -u/--usb [[<vid>,]<pid>]     Connect to target over USB HID device denoted by
								   vid/pid
									 (default=0x15a2,0x0073)
	  -V/--verbose                 Print extra detailed log information
	  -d/--debug                   Print really detailed log information
	  -j/--json                    Print output in JSON format to aid automation.
	  -n/--noping                  Skip the initial ping of a serial target
	  -t/--timeout <ms>            Set serial read timeout in milliseconds
									 (default=5000, max=25500)
	Command:
	  reset                        Reset the chip
	  get-property <tag>
		1                          Bootloader version
		2                          Available peripherals
		3                          Start of program flash
		4                          Size of program flash
		5                          Size of flash sector
		6                          Blocks in flash array
		7                          Available commands
		8                          CRC check status
		10                         Verify Writes flag
		11                         Max supported packet size
		12                         Reserved regions
		13                         Validate regions flag
		14                         Start of RAM
		15                         Size of RAM
		16                         System device identification
		17                         Flash security state
	  set-property <tag> <value>
		10                         Verify Writes flag
		13                         Validate regions flag
	  flash-erase-region <addr> <byte_count>
								   Erase a region of flash
	  flash-erase-all              Erase all flash, excluding protected regions
	  flash-erase-all-unsecure     Erase all flash, including protected regions
	  read-memory <addr> <byte_count> <file>
								   Read memory from file
	  write-memory <addr> [<file> | {{<hex-data>}}]
								   Write memory from file or string of hex values,
								   e.g. "{{11 22 33 44}}"
	  fill-memory <addr> <byte_count> <pattern> [word | short | byte]
								   Fill memory with pattern; size is
									 word (default), short or byte
	  receive-sb-file <file>       Receive SB file
	  execute <addr> <arg> <stackpointer>
								   Execute at address with arg and stack pointer
	  call <addr> <arg>            Call address with arg
	  flash-security-disable <key> Flash Security Disable <8-byte-hex-key>,
									 e.g. 0102030405060708

State files
===========

Applications that use the bootloader framework create files to store the state of simulated device memory and other persistent variables.
These state files are:

* state_mem<n>.dat - one binary file for each simulated memory, where "n" is the index in the memory map. For example, state_mem0.dat is simulated
Flash memory and state_mem1.dat is simulated SRAM on the KL25Z.
* state_init.dat - binary file with the initial values of persistent variables. For example, the property store.

These files are stored in the current working directory, but they can be redirected to another directory using the `--state` command line argument.
By default, applications create these files if they don't exist, and use them if they do exist. You can also force the application to recreate the
files by passing the `--init` command line argument.

Applications that run the bootloader simulator, such as BLSIM, use the memory files. If the memory files exist they are used to pre-initialize the
simulated memory stores before running any simulated bootloader commands. The state init file is used to pre-initialize the property store.

The BLHOST (firmware download) application does not use the state files.
