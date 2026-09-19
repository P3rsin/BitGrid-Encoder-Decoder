#include "BitGridCodec.h"

#include <iostream>
#include <string>
#include <variant>

using namespace std;

int failures = 0;

void expect(bool condition, const string &testName)
{
    if (!condition)
    {
        cerr << "FAIL: " << testName << '\n';
        failures++;
    }
}

void expectError(const CodecResult &result, CodecError expectedError, const string &testName)
{
    expect(holds_alternative<CodecError>(result) && get<CodecError>(result) == expectedError, testName);
}

void expectRoundTrip(BitGridCodec &codec, const string &input, const string &testName)
{
    CodecResult encoded = codec.encode(input);

    if (!holds_alternative<string>(encoded))
    {
        expect(false, testName + " (encode failed)");
        return;
    }

    CodecResult decoded = codec.decode(get<string>(encoded));

    if (!holds_alternative<string>(decoded))
    {
        expect(false, testName + " (decode failed)");
        return;
    }

    expect(get<string>(decoded) == input, testName);
}

int main()
{
    BitGridCodec codec;

    // Round-trip tests
    expectRoundTrip(codec, "", "empty string round trip");
    expectRoundTrip(codec, "Hello", "simple text round trip");
    expectRoundTrip(codec, "Hello world", "text with spaces round trip");
    expectRoundTrip(codec, "line one\nline two", "multiline text round trip");
    expectRoundTrip(codec, "line one\nline two\n", "trailing newline round trip");

    string binaryPayload;
    binaryPayload.push_back('\0');
    binaryPayload.push_back(static_cast<char>(0xFF));
    binaryPayload.push_back('A');

    expectRoundTrip(codec, binaryPayload, "binary byte values round trip");

    // Payload-size boundaries
    string maxPayload(65535, 'x');
    string oversizedPayload(65536, 'x');
    CodecResult maxPayloadResult = codec.encode(maxPayload);

    expect(holds_alternative<string>(maxPayloadResult), "65535-byte payload succeeds");
    expectError(codec.encode(oversizedPayload), CodecError::InputTooLarge, "65536-byte payload returns InputTooLarge");

    // Empty / incomplete input
    expectError(codec.decode(""), CodecError::MissingData, "empty grid returns MissingData");

    const string tinyGrid = "+------+\n|......|\n|......|\n|......|\n+------+\n";
    expectError(codec.decode(tinyGrid), CodecError::IncompleteHeader,
                "grid with fewer than 64 data bits returns IncompleteHeader");

    // Grid created to intentionally corrupt
    CodecResult encodedHelloResult = codec.encode("Hello");

    if (!holds_alternative<string>(encodedHelloResult))
    {
        cerr << "FAIL: setup encode for malformed-grid tests\n";
        return 1;
    }

    const string encodedHello = get<string>(encodedHelloResult);

    // Border validation

    string badTopBorder = encodedHello;
    badTopBorder[0] = 'o';

    expectError(codec.decode(badTopBorder), CodecError::MalformedBorder, "malformed top border is rejected");

    string badSideBorder = encodedHello;
    size_t firstNewline = badSideBorder.find('\n');
    badSideBorder[firstNewline + 1] = 'o';
    
    expectError(codec.decode(badSideBorder), CodecError::MalformedBorder, "malformed side border is rejected");

    // Final result
    if (failures == 0)
    {
        cout << "All tests passed\n";
        return 0;
    }

    cerr << failures << " test(s) failed\n";
    return 1;
}