/*!
    \file service.cpp
    \brief Asio service implementation
    \author Ivan Shynkarenka
    \date 16.12.2016
    \copyright MIT License
*/

#include "server/asio/service.h"

#include "errors/fatal.h"

#include <cassert>

namespace CppServer {
namespace Asio {

bool Service::Start(bool polling)
{
    assert(!IsStarted() && "Asio service is already started!");
    if (IsStarted())
        return false;

    // Post started routine
    auto self(this->shared_from_this());
    _service.post([this, self]()
    {
         // Update started flag
        _started = true;

        // Call service started handler
        onStarted();
    });

    // Start service thread
    _thread = std::thread([this, polling]() { ServiceLoop(polling); });

    return true;
}

bool Service::Stop()
{
    assert(IsStarted() && "Asio service is already stopped!");
    if (!IsStarted())
        return false;

    // Post stop routine
    auto self(this->shared_from_this());
    _service.post([this, self]()
    {
        // Update started flag
        _started = false;

        // Call service stopped handler
        onStopped();

        // Stop Asio service
        _service.stop();
    });

    // Wait for service thread
    _thread.join();

    return true;
}

void Service::ServiceLoop(bool polling)
{
    // Call initialize thread handler
    onThreadInitialize();

    try
    {
        asio::io_service::work work(_service);

        if (polling)
        {
            // Run Asio service in a polling loop
            do
            {
                // Poll all pending handlers
                _service.poll();

                // Call idle handler
                onIdle();
            } while (_started);
        }
        else
        {
            // Run all pending handlers
            _service.run();
        }
    }
    catch (asio::system_error& ex)
    {
        onError(ex.code().value(), ex.code().category().name(), ex.code().message());
    }
    catch (...)
    {
        fatality("TCP service thread terminated!");
    }

    // Call cleanup thread handler
    onThreadCleanup();
}

} // namespace Asio
} // namespace CppServer
