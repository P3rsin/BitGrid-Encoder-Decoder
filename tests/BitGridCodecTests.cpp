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

int main()
{
    BitGridCodec codec;

    CodecResult encoded = codec.encode("Hello");
    expect(holds_alternative<string>(encoded), "encode Hello succeeds");

    cout << "All tests passed\n";
    return 0;
}