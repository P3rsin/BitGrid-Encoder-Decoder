#include "BitGridCodec.h"

#include <algorithm>
#include <array>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace
{
enum class Command
{
    Encode,
    Decode
};

constexpr std::string_view help = R"(BitGrid Encoder/Decoder

Usage:
  bitgrid encode <text>        Encode one literal argument
  bitgrid encode -- <text>     Encode a reserved argument literally
  bitgrid encode -f <file>     Encode bytes from a file
  bitgrid encode -             Encode bytes from standard input
  bitgrid decode <file>        Decode a BitGrid file
  bitgrid decode -- <file>     Decode a file named like an option
  bitgrid decode -             Decode a BitGrid from standard input
  bitgrid help | --help        Display help
  bitgrid --version            Display the program version
)";

int error(std::string_view message)
{
    std::cerr << "bitgrid: error: " << message << '\n';
    return 1;
}

int usageError(std::string_view message)
{
    const int status = error(message);
    std::cerr << "Run 'bitgrid --help' for usage.\n";
    return status;
}

bool setBinaryMode([[maybe_unused]] std::FILE *stream)
{
#ifdef _WIN32
    const int fd = _fileno(stream);
    return fd >= 0 && _setmode(fd, _O_BINARY) != -1;
#else
    return true;
#endif
}

int writeOutput(std::string_view bytes)
{
    if (!setBinaryMode(stdout))
    {
        return error("could not set binary mode on standard output");
    }
#ifdef SIGPIPE
    // Report a closed downstream pipe as a write error.
    std::signal(SIGPIPE, SIG_IGN);
#endif
    if (std::fwrite(bytes.data(), 1, bytes.size(), stdout) != bytes.size() || std::fflush(stdout) != 0 ||
        std::ferror(stdout))
    {
        return error("could not write to standard output");
    }
    return 0;
}

std::size_t inputLimit(Command command)
{
    return command == Command::Encode ? BitGridCodec::maxPayloadSize : BitGridCodec::maxGridSize();
}

int inputTooLarge(Command command)
{
    const auto kind = command == Command::Encode ? "payload" : "grid";
    return error(std::string(kind) + " exceeds the maximum of " + std::to_string(inputLimit(command)) + " bytes");
}

int runCodec(Command command, std::string_view input)
{
    const auto result = command == Command::Encode ? BitGridCodec::encode(input) : BitGridCodec::decode(input);
    if (const auto *bytes = std::get_if<std::string>(&result))
    {
        return writeOutput(*bytes);
    }

    switch (std::get<CodecError>(result))
    {
    case CodecError::InputTooLarge:
        return inputTooLarge(command);
    case CodecError::IncompleteHeader:
        return error("incomplete BitGrid header");
    case CodecError::InvalidSignature:
        return error("invalid BitGrid signature");
    case CodecError::MissingData:
        return error("missing encoded data");
    case CodecError::ChecksumMismatch:
        return error("checksum mismatch");
    case CodecError::MalformedBorder:
        return error("malformed BitGrid border");
    case CodecError::InvalidDimensions:
        return error("invalid grid dimensions or line endings");
    case CodecError::InvalidBlock:
        return error("invalid bit block");
    case CodecError::NoncanonicalDimensions:
        return error("grid is larger than required by the declared payload length");
    case CodecError::InvalidPadding:
        return error("grid padding must use '##' blocks");
    }
    return error("unknown codec error");
}

int readInput(Command command, std::FILE *stream, const std::string &source)
{
    const auto limit = inputLimit(command);
    std::array<char, 8192> buffer;
    std::string input;
    for (;;)
    {
        // A one-byte lookahead distinguishes an exact-limit input from an oversized one.
        const auto count = std::min(buffer.size(), limit - input.size() + 1);
        const auto received = std::fread(buffer.data(), 1, count, stream);
        if (std::ferror(stream) || (received < count && !std::feof(stream)))
        {
            return error("could not read " + source);
        }
        if (received > limit - input.size())
        {
            return inputTooLarge(command);
        }
        input.append(buffer.data(), received);
        if (std::feof(stream))
        {
            return runCodec(command, input);
        }
    }
}

int readStdin(Command command)
{
    if (!setBinaryMode(stdin))
    {
        return error("could not set binary mode on standard input");
    }
    return readInput(command, stdin, "standard input");
}

int readFile(Command command, const char *path)
{
    std::FILE *file = nullptr;
#ifdef _MSC_VER
    if (fopen_s(&file, path, "rb") != 0)
    {
        file = nullptr;
    }
#else
    file = std::fopen(path, "rb");
#endif
    const auto closeFile = [](std::FILE *handle) { std::fclose(handle); };
    const std::unique_ptr<std::FILE, decltype(closeFile)> input(file, closeFile);
    const auto source = "file '" + std::string(path) + "'";
    if (!input)
    {
        return error("could not open " + source);
    }
    return readInput(command, input.get(), source);
}

int encodeCommand(int argc, char *const argv[])
{
    if (argc < 3)
    {
        return usageError("no input provided for encoding");
    }
    const std::string_view source = argv[2];
    if (source == "-f" && argc == 3)
    {
        return usageError("'-f' requires a file path");
    }
    if (argc == 4 && source == "-f")
    {
        return readFile(Command::Encode, argv[3]);
    }
    if (argc == 4 && source == "--")
    {
        return runCodec(Command::Encode, argv[3]);
    }
    if (argc != 3)
    {
        return usageError("invalid arguments for encode");
    }
    if (source == "-")
    {
        return readStdin(Command::Encode);
    }
    return runCodec(Command::Encode, source);
}

int decodeCommand(int argc, char *const argv[])
{
    if (argc < 3)
    {
        return usageError("no input provided for decoding");
    }
    const std::string_view source = argv[2];
    if (argc == 4 && source == "--")
    {
        return readFile(Command::Decode, argv[3]);
    }
    if (argc != 3)
    {
        return usageError("invalid arguments for decode");
    }
    if (source == "-")
    {
        return readStdin(Command::Decode);
    }
    return readFile(Command::Decode, argv[2]);
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return usageError("no command provided");
    }
    const std::string_view command = argv[1];
    if (command == "encode")
    {
        return encodeCommand(argc, argv);
    }
    if (command == "decode")
    {
        return decodeCommand(argc, argv);
    }
    if (command == "help" || command == "--help" || command == "--version")
    {
        if (argc != 2)
        {
            return usageError("extra arguments after " + std::string(command));
        }
        return writeOutput(command == "--version" ? "bitgrid " BITGRID_VERSION "\n" : help);
    }
    return usageError("unknown command '" + std::string(command) + "'");
}
