#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/json/api.h>
#include <arrow/csv/api.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

// Helper function to handle and log errors
void handle_status(const arrow::Status& status, const std::string& context) {
    if (!status.ok()) {
        std::cerr << context << " failed: " << status.ToString() << std::endl;
    }
}

// JSON Parsing Test
void test_json_parsing(const std::shared_ptr<arrow::Buffer>& buffer) {
    try {
        auto parse_options = arrow::json::ParseOptions::Defaults();
        auto read_options = arrow::json::ReadOptions::Defaults();

        auto maybe_result = arrow::json::TableReader::Make(
            arrow::default_memory_pool(),
            std::make_shared<arrow::io::BufferReader>(buffer),
            read_options,
            parse_options
        );

        if (maybe_result.ok()) {
            auto table_reader = maybe_result.ValueOrDie();
            auto table_result = table_reader->Read();  // Updated to match API
            if (table_result.ok()) {
                auto table = table_result.ValueOrDie();
                std::cout << "Parsed JSON into table with " << table->num_rows()
                          << " rows and " << table->num_columns() << " columns.\n";
            } else {
                std::cerr << "Reading JSON table failed: " << table_result.status().ToString() << std::endl;
            }
        } else {
            std::cerr << "JSON Parsing failed: " << maybe_result.status().ToString() << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "JSON Parsing Exception: " << ex.what() << std::endl;
    }
}

// CSV Parsing Test
void test_csv_parsing(const std::shared_ptr<arrow::Buffer>& buffer) {
    try {
        auto csv_options = arrow::csv::ReadOptions::Defaults();

        arrow::io::IOContext io_context(arrow::default_memory_pool());
        auto csv_reader_result = arrow::csv::TableReader::Make(
            io_context,
            std::make_shared<arrow::io::BufferReader>(buffer),
            csv_options,
            arrow::csv::ParseOptions::Defaults(),
            arrow::csv::ConvertOptions::Defaults()
        );

        if (csv_reader_result.ok()) {
            auto csv_table_reader = csv_reader_result.ValueOrDie();
            auto csv_table_result = csv_table_reader->Read();  // Updated to match API
            if (csv_table_result.ok()) {
                auto csv_table = csv_table_result.ValueOrDie();
                std::cout << "Parsed CSV into table with " << csv_table->num_rows()
                          << " rows and " << csv_table->num_columns() << " columns.\n";
            } else {
                std::cerr << "CSV Parsing failed: " << csv_table_result.status().ToString() << std::endl;
            }
        } else {
            std::cerr << "CSV Table Reader creation failed: " << csv_reader_result.status().ToString() << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "CSV Parsing Exception: " << ex.what() << std::endl;
    }
}

// Array Creation Test
void test_array_creation(const uint8_t* data, size_t size) {
    try {
        arrow::Int32Builder int_builder;

        for (int i = 0; i < size / sizeof(int); ++i) {
            auto status = int_builder.Append(reinterpret_cast<const int*>(data)[i]);
            handle_status(status, "Appending to Int32Builder");
        }

        std::shared_ptr<arrow::Array> int_array;
        auto status = int_builder.Finish(&int_array);
        handle_status(status, "Finishing Int32Builder");

        if (int_array) {
            std::cout << "Created integer array with " << int_array->length() << " elements.\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Array Creation Exception: " << ex.what() << std::endl;
    }
}

// Table Serialization Test
void test_table_serialization(const std::shared_ptr<arrow::Buffer>& buffer) {
    try {
        auto parse_options = arrow::json::ParseOptions::Defaults();
        auto read_options = arrow::json::ReadOptions::Defaults();

        auto maybe_result = arrow::json::TableReader::Make(
            arrow::default_memory_pool(),
            std::make_shared<arrow::io::BufferReader>(buffer),
            read_options,
            parse_options
        );

        if (maybe_result.ok()) {
            auto table_reader = maybe_result.ValueOrDie();
            auto table_result = table_reader->Read();  // Updated to match API
            if (table_result.ok()) {
                auto table = table_result.ValueOrDie();
                auto output_stream = arrow::io::BufferOutputStream::Create().ValueOrDie();
                auto writer = arrow::ipc::MakeFileWriter(output_stream, table->schema()).ValueOrDie();

                auto status = writer->WriteTable(*table);
                handle_status(status, "Writing table to stream");

                status = writer->Close();
                handle_status(status, "Closing writer");

                auto serialized_data = output_stream->Finish().ValueOrDie();
                std::cout << "Serialized table to buffer of size " << serialized_data->size() << ".\n";
            } else {
                std::cerr << "Table Serialization failed: " << table_result.status().ToString() << std::endl;
            }
        } else {
            std::cerr << "Table Serialization failed: " << maybe_result.status().ToString() << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Table Serialization Exception: " << ex.what() << std::endl;
    }
}

// Main Fuzzing Function
void fuzz(const uint8_t* data, size_t size) {
    auto buffer = std::make_shared<arrow::Buffer>(data, size);

    test_json_parsing(buffer);
    test_csv_parsing(buffer);
    test_array_creation(data, size);
    test_table_serialization(buffer);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

    fuzz(data.data(), data.size());
    return 0;
}
