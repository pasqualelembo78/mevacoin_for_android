// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2024, MevaCoin Project.
//
// Please see the included LICENSE file for more information.

#include "logger_message.h"

#include <utility>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace logging
{

    // Costruttore principale: associa lo streambuf (quest'oggetto) allo stream
    LoggerMessage::LoggerMessage(std::shared_ptr<ILogger> logger,
                                 const std::string &category,
                                 Level level,
                                 const std::string &color)
        : std::ostream(nullptr),
          std::streambuf(),
          logger(std::move(logger)),
          category(category),
          logLevel(level),
          message(color),
          timestamp(boost::posix_time::microsec_clock::local_time()),
          gotText(false)
    {
        // imposta questo oggetto come streambuf per lo stream base
        this->set_rdbuf(this);
    }

    // Distruttore: se abbiamo testo non ancora flush-ato, aggiungiamo newline
    LoggerMessage::~LoggerMessage()
    {
        // Se è rimasto testo non flushato, scriviamo newline (che attiva sync())
        if (gotText)
        {
            // use operator<< to trigger overflow/xsputn and then flush via endl
            (*this) << std::endl;
        }
        // disassociare il rdbuf (non strettamente necessario, ma pulisce lo stato)
        this->set_rdbuf(nullptr);
    }

    // Move-constructor portabile: sposta solo i membri "propri" (non manipola membri interni di std::ostream)
    LoggerMessage::LoggerMessage(LoggerMessage &&other) noexcept
        : std::ostream(nullptr),
          std::streambuf(),
          logger(std::move(other.logger)),
          category(std::move(other.category)),
          logLevel(other.logLevel),
          message(std::move(other.message)),
          timestamp(boost::posix_time::microsec_clock::local_time()),
          gotText(other.gotText)
    {
        // impostiamo questo come buffer dello stream
        this->set_rdbuf(this);

        // disattiviamo il comportamento del moved-from per evitare flush indesiderati
        other.gotText = false;
        other.set_rdbuf(nullptr);
    }

    // sync() viene chiamato quando lo stream viene flushato (es. std::endl).
    // Chiamiamo il logger effettivo con i dati accumulati.
    int LoggerMessage::sync()
    {
        if (logger)
        {
            try
            {
                (*logger)(category, logLevel, timestamp, message);
            }
            catch (...)
            {
                // Se il logger lancia, non interrompiamo il programma;
                // restituiamo comunque 0 (potresti decidere di restituire -1)
            }
        }
        gotText = false;
        message = DEFAULT;
        return 0;
    }

    // xsputn: scrive più caratteri alla volta nello streambuf
    std::streamsize LoggerMessage::xsputn(const char *s, std::streamsize n)
    {
        if (n > 0 && s != nullptr)
        {
            gotText = true;
            message.append(s, static_cast<size_t>(n));
        }
        return n;
    }

    // overflow: viene chiamato per un singolo carattere
    int LoggerMessage::overflow(int c)
    {
        if (c == EOF)
            return EOF;

        gotText = true;
        message.push_back(static_cast<char>(c));
        return c;
    }

} // namespace logging