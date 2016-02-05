#include <exception>
#include <iostream>
using namespace std;

#include <utility/config_file.h>
#include <utility/single_instance.h>
#include <utility/logger.h>
#include <utility/file_utility.h>
#include <server/server.h>

int main ()
{
    ///////////////////////////////////////////
    // Load configuration file
    vector<string> symbols;
    bool hyperThreading{true};
    bool centralOrderPinThreadsToCores{false};
    int centralOrderBookQueueSizePerThread{0};
    int centralOrderBookThreadStackSize{0};
    int singleInstanceTCPPortNumber{0};

    try
    {
        utility::ConfigFile configuration;
        configuration.loadFromFile("ome.ini");

        singleInstanceTCPPortNumber = configuration.getIntVaue("SINGLE_INSTANCE_TCP_PORT");
        
        utility::Logger::getInstance().initialise(configuration.getIntVaue("LOG_BUFFER_SIZE"));
        utility::Logger::getInstance().setLogFile("oms_log.txt");
        utility::Logger::getInstance().enableFileLogging(configuration.getBoolValue("FILE_LOGGING_ENABLED"));
        utility::Logger::getInstance().enableConsoleOutput(configuration.getBoolValue("CONSOLE_OUTPUT_ENABLED"));

        symbols = configuration.getArray("SYMBOL");
        if (symbols.size() == 0) { throw std::runtime_error("No symbol found in the ini file"); }

        centralOrderPinThreadsToCores = configuration.getBoolValue("CENTRAL_ORDER_BOOK_PIN_THREADS_TO_CORES");
        hyperThreading = configuration.getBoolValue("HYPER_THREADING");
        centralOrderBookQueueSizePerThread = configuration.getIntVaue("CENTRAL_ORDER_BOOK_WORK_QUEUE_SIZE_PER_THREAD");
        centralOrderBookThreadStackSize = configuration.getIntVaue("CENTRAL_ORDER_BOOK_THREAD_STACK_SIZE");
    }
    catch (std::runtime_error& e)
    {
        cerr << e.what() << endl;
        return 2;
    }
    
     //////////////////////////////////////////
    // Single instance protection
    utility::SingleInstance singleton(singleInstanceTCPPortNumber);
    
    if ( !singleton() )
    {
        cerr << "Ome process is running already." << endl;
        return 1;
    }

    //////////////////////////////////////////
    // Backup FIX engine logs if exists
    utility::createDirectory("old_quickfix_logs");
    utility::backupDirectory("quickfix_log", "quickfix_log_" + utility::getCurrentDateTime("%d_%m_%Y_%H_%M_%S"), "old_quickfix_logs");
    //////////////////////////////////////////

    try
    {
        // Start logger
        utility::Logger::getInstance().start();
        LOG_INFO("Main thread", "starting")

        Server application( "quickfix_server.cfg",
                          centralOrderPinThreadsToCores,
                          centralOrderBookThreadStackSize,
                          hyperThreading,
                          centralOrderBookQueueSizePerThread,
                          symbols
                          );

        // Run the server
        application.run();
    }
    catch (std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        return 3;
    }

    //////////////////////////////////////////
    // Application exit
    LOG_INFO("Main thread", "Ending")
    utility::Logger::getInstance().shutdown();
    return 0;
}