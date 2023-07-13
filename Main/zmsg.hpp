#ifndef __ZMSG_H_INCLUDED__
#define __ZMSG_H_INCLUDED__

#include "zhelpers.hpp"

#include <vector>
#include <string>
#include <stdarg.h>

#include <QByteArray>
#include <QString>



class zmsg {
public:
//    typedef std::basic_string<unsigned char> ustring;
    typedef QByteArray ustring;

    zmsg() {
    }

    //  --------------------------------------------------------------------------
    //  Constructor, sets initial body
    zmsg(char const *body) {
        body_set(body);
    }

    //  -------------------------------------------------------------------------
    //  Constructor, sets initial body and sends message to socket
    zmsg(char const *body, zmq::socket_t &socket) {
        body_set(body);
        send(socket);
    }

    //  --------------------------------------------------------------------------
    //  Constructor, calls first receive automatically
    zmsg(zmq::socket_t &socket) {
        recv(socket);
    }

    //  --------------------------------------------------------------------------
    //  Copy Constructor, equivalent to zmsg_dup
    zmsg(zmsg &msg) {
        m_part_data.resize(msg.m_part_data.size());
        std::copy(msg.m_part_data.begin(), msg.m_part_data.end(), m_part_data.begin());
    }

    virtual ~zmsg() {
        clear();
    }

    //  --------------------------------------------------------------------------
    //  Erases all messages
    void clear() {
        m_part_data.clear();
    }

    void set_part(size_t part_nbr, unsigned char *data) {
        if (part_nbr < m_part_data.size()) {
            m_part_data[part_nbr] = QByteArray((char*)data, std::strlen((char*)data));
        }
    }

    bool recv(zmq::socket_t & socket) {
        clear();
        while(1) {
            zmq::message_t message(0);
            try {
                if (!socket.recv(message)) {
                    return false;
                }
            } catch (zmq::error_t error) {
                std::cout << "E: " << error.what() << std::endl;
                return false;
            }
            //std::cerr << "recv: \"" << (unsigned char*) message.data() << "\", size " << message.size() << std::endl;
            if (message.size() == 17 && ((unsigned char *)message.data())[0] == 0) {
                char *uuidstr = encode_uuid((unsigned char*) message.data());
                push_back(uuidstr);
                delete[] uuidstr;
            }
            else {
                m_part_data.push_back(ustring((char*) message.data(), message.size()));
            }
            if (!message.more()) {
                break;
            }
        }
        return true;
    }

    void send(zmq::socket_t & socket) {
        for (size_t part_nbr = 0; part_nbr < m_part_data.size(); part_nbr++) {
            zmq::message_t message;
            ustring data = m_part_data[part_nbr];
            if (data.size() == 33 && data [0] == '@') {
                unsigned char * uuidbin = decode_uuid ((char *) data.data());
                message.rebuild(17);
                memcpy(message.data(), uuidbin, 17);
                delete uuidbin;
            }
            else {
                message.rebuild(data.size());
                memcpy(message.data(), data.data(), data.size());
            }
            try {
                socket.send(message, part_nbr < m_part_data.size() - 1 ? zmq::send_flags::sndmore : zmq::send_flags::none);
            } catch (zmq::error_t error) {
                assert(error.num()!=0);
            }
        }
        clear();
    }

    size_t parts() {
        return m_part_data.size();
    }

    void body_set(const char *body) {
        if (m_part_data.size() > 0) {
            m_part_data.erase(m_part_data.end()-1);
        }
        push_back((char*)body);
    }

    void body_set(QByteArray body) {
        if (m_part_data.size() > 0) {
            m_part_data.erase(m_part_data.end()-1);
        }
        push_back(body);
    }


    void
    body_fmt (const char *format, ...)
    {
        char value [255 + 1];
        va_list args;

        va_start (args, format);
        vsnprintf (value, 255, format, args);
        va_end (args);

        body_set (value);
    }

    char * body ()
    {
        if (m_part_data.size())
            return ((char *) m_part_data [m_part_data.size() - 1].data());
        else
            return 0;
    }

    // zmsg_push
    void push_front(char *part) {
        m_part_data.insert(m_part_data.begin(), QByteArray(part, std::strlen(part)));
    }

    // zmsg_append
    void push_back(char *part) {
        m_part_data.push_back(QByteArray(part, std::strlen(part)));
    }


    // zmsg_push
    void push_front(QByteArray part) {
        m_part_data.insert(m_part_data.begin(), part);
    }

    // zmsg_append
    void push_back(QByteArray part) {
        m_part_data.push_back(part);
    }

    //  --------------------------------------------------------------------------
    //  Formats 17-byte UUID as 33-char string starting with '@'
    //  Lets us print UUIDs as C strings and use them as addresses
    //
    static char *
    encode_uuid (unsigned char *data)
    {
        static char
            hex_char [] = "0123456789ABCDEF";

        assert (data [0] == 0);
        char *uuidstr = new char[34];
        uuidstr [0] = '@';
        int byte_nbr;
        for (byte_nbr = 0; byte_nbr < 16; byte_nbr++) {
            uuidstr [byte_nbr * 2 + 1] = hex_char [data [byte_nbr + 1] >> 4];
            uuidstr [byte_nbr * 2 + 2] = hex_char [data [byte_nbr + 1] & 15];
        }
        uuidstr [33] = 0;
        return (uuidstr);
    }


    // --------------------------------------------------------------------------
    // Formats 17-byte UUID as 33-char string starting with '@'
    // Lets us print UUIDs as C strings and use them as addresses
    //
    static unsigned char *
    decode_uuid (char *uuidstr)
    {
        static char
            hex_to_bin [128] = {
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
               0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1, /* 0..9 */
               -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* A..F */
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* */
               -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, /* a..f */
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }; /* */

        assert (strlen (uuidstr) == 33);
        assert (uuidstr [0] == '@');
        unsigned char *data = new unsigned char[17];
        int byte_nbr;
        data [0] = 0;
        for (byte_nbr = 0; byte_nbr < 16; byte_nbr++)
            data [byte_nbr + 1]
                = (hex_to_bin [uuidstr [byte_nbr * 2 + 1] & 127] << 4)
                  + (hex_to_bin [uuidstr [byte_nbr * 2 + 2] & 127]);

        return (data);
    }

    // zmsg_pop
    ustring pop_front() {
        if (m_part_data.size() == 0) {
            return ustring();
        }
        ustring part = m_part_data.front();
        m_part_data.erase(m_part_data.begin());
        return part;
    }

    void append (QByteArray part)
    {
        assert (!part.isNull());
        assert (!part.isEmpty());

        push_back(part);
    }

    void append (const char *part)
    {
        assert (part);
        push_back((char*)part);
    }

    QString address() {
        if (m_part_data.size()>0) {
            return m_part_data[0];
        } else {
            return QString();
        }
    }

    void wrap(const char *address, const char *delim) {
        if (delim) {
            push_front((char*)delim);
        }
        push_front((char*)address);
    }


    void wrap(QString address, QString delim = QString("")) {
        if (!delim.isNull()) {
            push_front(delim.toLatin1());
        }
        push_front(address.toLatin1());
    }

    QString unwrap() {
        if (m_part_data.size() == 0) {
            return QString();
        }
        QString addr = pop_front();
        if (address().isEmpty()) {
            pop_front();
        }
        return addr;
    }

    void dump() {
        std::cerr << "--------------------------------------" << std::endl;
        for (unsigned int part_nbr = 0; part_nbr < m_part_data.size(); part_nbr++) {
            ustring data = m_part_data [part_nbr];

            // Dump the message as text or binary
            int is_text = 1;
            for (unsigned int char_nbr = 0; char_nbr < data.size(); char_nbr++)
                if (data [char_nbr] < 32 || data [char_nbr] > 127)
                    is_text = 0;

            std::cerr << "[" << std::setw(3) << std::setfill('0') << (int) data.size() << "] ";
            for (unsigned int char_nbr = 0; char_nbr < data.size(); char_nbr++) {
                if (is_text) {
                    std::cerr << (char) data [char_nbr];
                } else {
                    std::cerr << std::hex << std::setw(2) << std::setfill('0') << (short int) data [char_nbr];
                }
            }
            std::cerr << std::endl;
        }
    }



private:
    std::vector<ustring> m_part_data;

};

#endif /* ZMSG_H_ */
