/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/**
 * @file
 * @brief IronBee --- Eudoxus Compiler
 *
 * A compiler for Eudoxus.  Reads intermediate format files in the automata
 * protobuf format and produces Eudoxus automata files in native endianness.
 *
 * See intermediate.proto for file format.  See eudoxus.h for execution
 * engine.
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 */

#include <ironautomata/eudoxus_compiler.hpp>

#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>

using namespace std;
using namespace IronAutomata;

int main(int argc, char **argv)
{
    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    string output_s;
    string input_s;

    po::options_description desc("Options:");
    desc.add_options()
        ("help", "display help and exit")
        ("output,o", po::value<string>(&output_s),
            "where to write output, defaults to INPUT.e"
        )
        ("input,i", po::value<string>(&input_s),
            "where to read input from; required; but -i is optional"
        )
        ;

    po::positional_options_description pd;
    pd.add("input", 1);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
            .options(desc)
            .positional(pd)
            .run(),
        vm
    );
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }

    if (! vm.count("input")) {
        cout << "Input is required." << endl;
        cout << desc << endl;
        return 1;
    }

    fs::path input(input_s);
    fs::path output(output_s);

    if (! vm.count("output")) {
        output = input;
        output.replace_extension(".e");
    }

    fs::ifstream input_stream(input);
    if (! input_stream) {
        cout << "Error: Could not open " << input_s << " for reading."
             << endl;
        return 1;
    }

    fs::ofstream output_stream(output);
    if (! output_stream) {
        cout << "Error: Could not open " << output_s << " for writing."
             << endl;
        return 1;
    }

    Intermediate::automata_t automata;
    bool success = false;
    try {
        success = Intermediate::read_automata(
            automata,
            input_stream,
            ostream_logger(cout)
        );
        EudoxusCompiler::result_t result =
            EudoxusCompiler::compile_minimal(automata);

        cout << "bytes    = " << result.buffer.size() << endl;
        cout << "id_width = " << result.id_width << endl;
        cout << "ids_used = " << result.ids_used << endl;

        output_stream.write(result.buffer.data(), result.buffer.size());
        if (! output_stream) {
            cout << "Error: Error writing output." << endl;;
            success = false;
        }
    }
    catch (const boost::exception& e) {
        cout << "Error: Exception:" << endl;
        cout << diagnostic_information(e) << endl;
    }
    catch (const exception& e) {
        cout << "Error: Exception:" << endl;
        cout << e.what() << endl;
    }
    catch (...) {
        cout << "Error: Unknown Exception" << endl;
    }

    return (success ? 0 : 1);
}