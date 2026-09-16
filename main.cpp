#include "BitGridCodec.h"
#include <iostream>

using namespace std;

int printHelp()
{
    cout << "[help text here]\n";
    return 0;
}

int aboutProject()
{
    cout << "[Project description]" << '\n';
    return 0;
}

int handleEncoding(int argc, char *argv[])
{
    BitGridCodec bitGridCodec;

    if (argc < 3)
    {
        cout << "add some input text\n";
        return 1;
    }

    string arg2 = argv[2];
    cout << bitGridCodec.encode(arg2);

    return 0;
}

int handleDecoding(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "add some input text\n";
        return 1;
    }

    BitGridCodec bitGridCodec;

    string total;
    string line;
    while (getline(cin, line))
    {
        total = total + line + '\n';
    }

    cout << bitGridCodec.decode(total) << '\n';

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "use the help command for help\n";
        return 1;
    }

    string command = argv[1];

    if (command == "encode")
    {
        return handleEncoding(argc, argv);
    }
    else if (command == "decode")
    {
        return handleDecoding(argc, argv);
    }
    else if (command == "about")
    {
        return aboutProject();
    }
    else if (command == "help")
    {
        return printHelp();
    }
    else
    {
        cout << "Invalid input\n";
        return 1;
    }
}
