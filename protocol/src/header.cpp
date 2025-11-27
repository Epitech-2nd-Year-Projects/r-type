#include "protocol/header.h"
// #include "engine/net/buffer_reader.h"

bool protocol::EncodeHeader(const Header& header, engine::net::BufferWriter& writer) {
    // To be implemented after implementing BufferWriter methods.
    return false;
}

bool protocol::DecodeHeader(engine::net::BufferReader& reader, Header& out_header) {
    // To be implemented after implementing BufferReader methods.
    return false;
}
