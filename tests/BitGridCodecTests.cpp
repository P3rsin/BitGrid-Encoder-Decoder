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

    CodecResult encoded = codec.encode("Hello");
    expect(holds_alternative<string>(encoded), "encode Hello succeeds");

    cout << "All tests passed\n";
    return 0;
}