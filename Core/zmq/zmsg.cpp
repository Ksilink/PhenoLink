#include "zmsg.hpp"


int s_interrupted = 0;

QMutex zmsg::session_locker;

bool zmsg::recv(zmq::socket_t &socket) {
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

void zmsg::send(zmq::socket_t &socket) {
    session_locker.lock();
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
    session_locker.unlock();
}

char *zmsg::encode_uuid(unsigned char *data)
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

unsigned char *zmsg::decode_uuid(char *uuidstr)
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

void zmsg::dump() {
    std::cerr << "--------------------------------------" << std::endl;
    for (unsigned int part_nbr = 0; part_nbr < m_part_data.size(); part_nbr++) {
        ustring data = m_part_data [part_nbr];

        // Dump the message as text or binary
        int is_text = 1;
        for (unsigned int char_nbr = 0; char_nbr < data.size(); char_nbr++)
            if (data [char_nbr] < 32 || data [char_nbr] > 127)
                is_text = 0;

        std::cerr << "[" << std::setw(3) << std::setfill('0') << (int) data.size() << "] ";
        unsigned int max =  data.size() > 28 ? 28 : data.size();
        for (unsigned int char_nbr = 0; char_nbr < max; char_nbr++) {
            if (is_text) {
                std::cerr << (char) data [char_nbr];
            } else {
                std::cerr << std::hex << std::setw(2) << std::setfill('0') << (short int) data [char_nbr];
            }
        }
        std::cerr << std::endl;
    }
}
