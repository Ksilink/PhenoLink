#ifndef __ZMSG_H_INCLUDED__
#define __ZMSG_H_INCLUDED__

#include <algorithm>

#include "zhelpers.hpp"

#include <vector>
#include <string>
#include <stdarg.h>

#include <QByteArray>
#include <QString>


#include <QMutex>

#include <Dll.h>



class DllCoreExport zmsg {
public:
//    typedef std::basic_string<unsigned char> ustring;
    typedef QByteArray ustring;
    static QMutex session_locker;

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

    bool recv(zmq::socket_t & socket);

    void send(zmq::socket_t & socket);

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
    encode_uuid (unsigned char *data);


    // --------------------------------------------------------------------------
    // Formats 17-byte UUID as 33-char string starting with '@'
    // Lets us print UUIDs as C strings and use them as addresses
    //
    static unsigned char *
    decode_uuid (char *uuidstr);

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

    void dump();



private:
    std::vector<ustring> m_part_data;

};

#endif /* ZMSG_H_ */
