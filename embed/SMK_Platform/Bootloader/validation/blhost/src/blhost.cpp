/*
 * Copyright (c) 2013-14, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstring>
#include <algorithm>
#include <stdio.h>
#include "fsl_platform_status.h"
#include "blfwk/format_string.h"
#include "blfwk/host_bootloader.h"
#include "blfwk/uart_peripheral.h"
#include "blfwk/sim_peripheral.h"
#include "blfwk/bus_pal_peripheral.h"
#include "blfwk/usb_hid_packetizer.h"
#include "blfwk/serial_packetizer.h"
#include "blfwk/host_memory.h"
#include "blfwk/options.h"
#include "blfwk/utils.h"
#include "blfwk/Logging.h"
#include "bootloader/context.h"
#include "bootloader/bootloader.h"

using namespace blfwk;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

//! @brief The tool's name.
const char k_toolName[] = "blhost";

//! @brief Current version number for the tool.
const char k_version[] = "0.0.2";

//! @brief Copyright string.
const char k_copyright[] = "Copyright (c) 2013-14 Freescale Semiconductor, Inc.\nAll rights reserved.";

//! @brief Command line option definitions.
static const char * k_optionsDefinition[] = {
    "?|help",
    "v|version",
    "p:port <name>[,<speed>]",
    "b:buspal spi[,<speed>,<polarity>,<phase>,lsb|msb] | i2c,[<address>,<speed>]",
    "u?usb [[<vid>,]<pid>]",
    "V|verbose",
    "d|debug",
    "j|json",
    "n|noping",
    "t:timeout <ms>",
    NULL
};

//! @brief Usage text.
const char k_optionUsage[] = "\nOptions:\n\
  -?/--help                    Show this help\n\
  -v/--version                 Display tool version\n\
  -p/--port <name>[,<speed>]   Connect to target over UART. Specify COM port\n\
                               and optionally baud rate\n\
                                 (default=COM1,57600)\n\
                                 If -b, then port is BusPal port\n\
  -b/--buspal spi[,<speed>,<polarity>,<phase>,lsb|msb] |\n\
              i2c[,<address>,<speed>]\n\
                               Use SPI or I2C for BusPal<-->Target link\n\
                               All parameters between square brackets are\n\
                               optional, but preceding parameters must be\n\
                               present or marked with a comma.\n\
                               (ex. -b spi,1000,0,1) (ex. -b spi,1000,,lsb)\n\
                                 spi: speed(KHz),\n\
                                      polarity(0=active_high | 1=active_low),\n\
                                      phase(0=rising_edge | 1=falling_edge),\n\
                                      \"lsb\" | \"msb\"\n\
                                      (default=100,1,1,msb)\n\
                                 i2c: address(7-bit hex), speed(KHz)\n\
                                      (default=0x10,100)\n\
  -u/--usb [[<vid>,]<pid>]     Connect to target over USB HID device denoted by\n\
                               vid/pid\n\
                                 (default=0x15a2,0x0073)\n\
  -V/--verbose                 Print extra detailed log information\n\
  -d/--debug                   Print really detailed log information\n\
  -j/--json                    Print output in JSON format to aid automation.\n\
  -n/--noping                  Skip the initial ping of a serial target\n\
  -t/--timeout <ms>            Set serial read timeout in milliseconds\n\
                                 (default=5000, max=25500)\n";

//! @brief Trailer usage text that gets appended after the options descriptions.
static const char* usageTrailer = "-- command <args...>";

/*!
 * \brief Class that encapsulates the blhost tool.
 *
 * A single global logger instance is created during object construction. It is
 * never freed because we need it up to the last possible minute, when an
 * exception could be thrown.
 */
class BlHost
{
public:
    /*!
     * Constructor.
     *
     * Creates the singleton logger instance.
     */
    BlHost(int argc, char * argv[])
        :    m_argc(argc),
        m_argv(argv),
        m_cmdv(),
        m_comPort("COM1"),
        m_comSpeed(57600),
        m_useBusPal(false),
        m_busPalConfig(),
        m_logger(NULL),
        m_useUsb(false),
        m_usbVid(UsbHidPacketizer::kDefault_Vid),
        m_usbPid(UsbHidPacketizer::kDefault_Pid),
        m_serialReadTimeoutMs(5000),
        m_noPing(false)
    {
        // create logger instance
        m_logger = new StdoutLogger();
        m_logger->setFilterLevel(Logger::kInfo);
        Log::setLogger(m_logger);
    }

    //! @brief Destructor.
    virtual ~BlHost() {}

    //! @brief Run the application.
    int run();

protected:
    //! @brief Process command line options.
    int processOptions();

protected:
    int m_argc;                         //!< Number of command line arguments.
    char ** m_argv;                     //!< Command line arguments.
    string_vector_t m_cmdv;             //!< Command line argument vector.
    string m_comPort;                   //!< COM port to use.
    int  m_comSpeed;                    //!< COM port speed.
    bool m_useBusPal;                   //!< True if using BusPal peripheral.
    string_vector_t m_busPalConfig;     //!< Bus pal peripheral-specific argument vector.
    bool m_useUsb;                      //!< Connect over USB HID.
    uint16_t m_usbVid;                  //!< USB VID of the target HID device
    uint16_t m_usbPid;                  //!< USB PID of the target HID device
    bool m_noPing;                      //!< If true will not send the initial ping to a serial device
    uint32_t m_serialReadTimeoutMs;     //!< Serial read timeout in milliseconds.
    ping_response_t m_pingResponse;     //!< Response to initial ping
    StdoutLogger * m_logger;            //!< Singleton logger instance.
};

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

//! @brief Print command line usage.
static void printUsage()
{
    printf(k_optionUsage);
    printf(k_commandUsage);
}

int BlHost::processOptions()
{
    Options options(* m_argv, k_optionsDefinition);

    // If there are no arguments print the usage.
    if (m_argc == 1)
    {
        options.usage(std::cout, usageTrailer);
        printUsage();
        return 0;
    }

    OptArgvIter iter(--m_argc, ++m_argv);

    // Process command line options.
    int optchar;
    const char * optarg;
    while ((optchar = options(iter, optarg)))
    {
        switch (optchar)
        {
            case '?':
                options.usage(std::cout, usageTrailer);
                printUsage();
                return 0;

            case 'v':
                printf("%s %s\n%s\n", k_toolName, k_version, k_copyright);
                return 0;
                break;

            case 'p':
            {
                if (optarg)
                {
                    string_vector_t params = utils::string_split(optarg, ',');
                    m_comPort = params[0];
                    if (params.size() == 2)
                    {
                        int speed = atoi(params[1].c_str());
                        if (speed)
                        {
                            m_comSpeed =  speed;
                        }
                    }
                }
                else
                {
                    Log::error("Error: You must specify the COM port identifier string with the -p/--port option.\n");
                    options.usage(std::cout, usageTrailer);
                    printUsage();
                    return 0;
                }
                break;
            }
            case 'b':
            {
                if (optarg)
                {
                    m_busPalConfig = utils::string_split(optarg, ',');
                    if ((strcmp(m_busPalConfig[0].c_str(), "spi") != 0) &&
                        (strcmp(m_busPalConfig[0].c_str(), "i2c") != 0))
                    {
                        Log::error("Error: %s is not valid for option -b/--buspal.\n", m_busPalConfig[0].c_str());
                        options.usage(std::cout, usageTrailer);
                        printUsage();
                        return 0;
                    }

                    m_useBusPal = true;
                }
                break;
            }
            case 'u':
            {
                if (optarg)
                {
                    string_vector_t params = utils::string_split(optarg, ',');
                    uint16_t vid = 0, pid = 0;
                    if (params.size() == 1)
                    {
                        pid = (uint16_t)strtoul(params[0].c_str(), NULL, 16);
                    }
                    else if (params.size() == 2)
                    {
                        vid = (uint16_t)strtoul(params[0].c_str(), NULL, 16);
                        pid = (uint16_t)strtoul(params[1].c_str(), NULL, 16);
                    }
                    if (vid)
                    {
                        m_usbVid =  vid;
                    }
                    if (pid)
                    {
                        m_usbPid =  pid;
                    }
                }
                m_useUsb = true;
                break;
            }

            case 'V':
                Log::getLogger()->setFilterLevel(Logger::kDebug);
                break;

            case 'd':
                Log::getLogger()->setFilterLevel(Logger::kDebug2);
                break;

            case 'j':
                Log::getLogger()->setFilterLevel(Logger::kJson);
                break;

            case 'n':
                m_noPing = true;
                break;

            case 't':
                if (optarg)
                {
                    // Max serial read timeout using termios is 25.5 sec.
                    m_serialReadTimeoutMs = min(strtoul(optarg, NULL, 0), (unsigned long)25500);
                }
                break;

            // All other cases are errors.
            default:
                return 1;
        }
    }

    // Treat the rest of the command line as a single bootloader command,
    // possibly with arguments.
    if (iter.index() == m_argc)
    {
        options.usage(std::cout, usageTrailer);
        printUsage();
        return 0;
    }

    // Save command name and arguments.
    for (int i = iter.index(); i < m_argc; ++i)
    {
        m_cmdv.push_back(m_argv[i]);
    }

    // All is well.
    return -1;
}

int BlHost::run()
{
    status_t result = kStatus_Success;
    Peripheral::PeripheralConfigData config;
    Command * cmd = NULL;

    // Read command line options.
    int optionsResult;
    if ((optionsResult = processOptions()) != -1)
    {
        return optionsResult;
    }

    try
    {
        if (m_cmdv.size())
        {
            //Check for any passed commands and validate command.
            cmd = Command::create(&m_cmdv);
            if (!cmd)
            {
                std::string msg = format_string("Error: invalid command or arguments '%s", m_cmdv.at(0).c_str());
                string_vector_t::iterator it = m_cmdv.begin();
                for (++it; it != m_cmdv.end(); ++it)
                {
                    msg.append(format_string(" %s", (*it).c_str()));
                }
                msg.append("'\n");
                throw std::runtime_error(msg);
            }
        }

        if (m_useUsb)
        {
            config.peripheralType = Peripheral::kHostPeripheralType_USB_HID;
            config.usbHidVid = m_usbVid;
            config.usbHidPid = m_usbPid;
        }
        else
        {
            config.peripheralType = Peripheral::kHostPeripheralType_UART;
            config.comPortName = m_comPort.c_str();
            config.comPortSpeed = m_comSpeed;
            config.serialReadTimeoutMs = m_serialReadTimeoutMs;
            if (m_useBusPal)
            {
                config.peripheralType = Peripheral::kHostPeripheralType_BUSPAL_UART;
                BusPal::parse(m_busPalConfig, config.busPalConfig);
            }
        }

        // Init the Bootloader object.
        Bootloader * bl = Bootloader::create(config);

        if (cmd)
        {
            // If we have a command inject it.
            Log::info("Inject command '%s'\n", cmd->getName().c_str());
            bl->inject(*cmd);
            bl->flush();

            // Print command response values.
            cmd->logResponses();

            // Only thing we consider an error is NoResponse
            if ( cmd->getResponseValues()->at(0) == kStatus_NoResponse )
            {
                result = kStatus_NoResponse;
            }

            delete cmd;
        }
    }
    catch (exception& e)
    {
//        cout << e.what();
        Log::error(e.what());
        result = kStatus_Fail;
    }

    return result;
}

//! @brief Application entry point.
int main(int argc, char * argv[], char * envp[])
{
    return BlHost(argc, argv).run();
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////

