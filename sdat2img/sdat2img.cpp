#include <iostream>
#include <string>
#include <vector>
#include <tchar.h>
#include <cstring>
#include <fstream>
extern "C"
{
#include "../common/constants.h"
#include "../common/version.h"
#include <brotli/decode.h>
}
#include "WSTRUtil.h"
using namespace std;
using namespace WSTRUtils;
#define BLOCK_SIZE 4096
static const size_t kFileBufferSize = 1 << 19;

typedef struct {
    uint8_t* buffer;
    uint8_t* input;
    uint8_t* output;
    wstring current_input_path;
    wstring current_output_path;
    FILE* fin;
    FILE* fout;

    /* I/O buffers */
    size_t available_in;
    const uint8_t* next_in;
    size_t available_out;
    uint8_t* next_out;

    /* Reporting */
    /* size_t would be large enough,
       until 4GiB+ files are compressed / decompressed on 32-bit CPUs. */
    size_t total_in;
    size_t total_out;
} Context;

void safe_write(const void* data, size_t count, FILE* img) {
    if (count != fwrite(data, 1, count, img)) {
        cout << "!!!writing error!!!" << endl;
        system("pause");
        exit(-1);
    }
}

void safe_read(void* data, size_t count, FILE* img) {
    if (count != fread(data, 1, count, img)) {
        cout << "!!!reading error!!!" << endl;
        system("pause");
        exit(-1);
    }
}

void safe_error(wstring prefix, wstring suffix) {
    cout << wchar_to_Cp1251(prefix) << " " << wchar_to_Cp1251(suffix) << endl;
    system("pause");
    exit(-1);
}

void safe_error(wstring prefix) {
    cout << wchar_to_Cp1251(prefix) << endl;
    system("pause");
    exit(-1);
}

static void PrintBytes(long long value) {
    if (value < 1024) {
        fprintf(stderr, "%d B", (int)value);
    }
    else if (value < 1048576) {
        fprintf(stderr, "%0.3f KiB", (double)value / 1024.0);
    }
    else if (value < 1073741824) {
        fprintf(stderr, "%0.3f MiB", (double)value / 1048576.0);
    }
    else {
        fprintf(stderr, "%0.3f GiB", (double)value / 1073741824.0);
    }
}

static BROTLI_BOOL ProvideInput(Context* context) {
    context->available_in =
        fread(context->input, 1, kFileBufferSize, context->fin);
    context->total_in += context->available_in;
    context->next_in = context->input;
    if (ferror(context->fin)) {
        return BROTLI_FALSE;
    }
    return BROTLI_TRUE;
}

static void InitializeBuffers(Context* context) {
    context->available_in = 0;
    context->next_in = NULL;
    context->available_out = kFileBufferSize;
    context->next_out = context->output;
    context->total_in = 0;
    context->total_out = 0;
}

static string PrintablePath(const wstring path) {
    return wchar_to_Cp1251(path);
}

static BROTLI_BOOL WriteOutput(Context* context) {
    size_t out_size = (size_t)(context->next_out - context->output);
    context->total_out += out_size;
    if (out_size == 0) return BROTLI_TRUE;

    fwrite(context->output, 1, out_size, context->fout);
    if (ferror(context->fout)) {
        fprintf(stderr, "failed to write output [%s]: %s\n",
            PrintablePath(context->current_output_path).c_str(), strerror(errno));
        return BROTLI_FALSE;
    }
    return BROTLI_TRUE;
}

static BROTLI_BOOL ProvideOutput(Context* context) {
    if (!WriteOutput(context)) return BROTLI_FALSE;
    context->available_out = kFileBufferSize;
    context->next_out = context->output;
    return BROTLI_TRUE;
}

static BROTLI_BOOL HasMoreInput(Context* context) {
    return feof(context->fin) ? BROTLI_FALSE : BROTLI_TRUE;
}

static void PrintFileProcessingProgress(Context* context) {
    fprintf(stderr, "[%s]: ", PrintablePath(context->current_input_path).c_str());
    PrintBytes(context->total_in);
    fprintf(stderr, " -> ");
    PrintBytes(context->total_out);
}

static BROTLI_BOOL FlushOutput(Context* context) {
    if (!WriteOutput(context)) return BROTLI_FALSE;
    context->available_out = 0;
    return BROTLI_TRUE;
}

void DecompressFile(Context* context, BrotliDecoderState* s) {
    BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
    InitializeBuffers(context);
    for (;;) {
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            if (!HasMoreInput(context)) {
                fprintf(stderr, "corrupt input [%s]\n",
                    PrintablePath(context->current_input_path).c_str());
                return;
            }
            if (!ProvideInput(context)) return;
        }
        else if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            if (!ProvideOutput(context)) return;
        }
        else if (result == BROTLI_DECODER_RESULT_SUCCESS) {
            if (!FlushOutput(context)) return;
            if (context->available_in != 0 || HasMoreInput(context)) {
                fprintf(stderr, "corrupt input [%s]\n",
                    PrintablePath(context->current_input_path).c_str());
                return;
            }

            fprintf(stderr, "Decompressed ");
            PrintFileProcessingProgress(context);
            fprintf(stderr, "\n");
            return;
        }
        else {
            fprintf(stderr, "corrupt input [%s]\n",
                PrintablePath(context->current_input_path).c_str());
            return;
        }

        result = BrotliDecoderDecompressStream(s, &context->available_in,
            &context->next_in, &context->available_out, &context->next_out, 0);
    }
}

void DecomressBrotli(Context* context, wstring file_path, wstring prefix) {
    cout << "Decomress Brotli..." << endl;
    context->current_input_path = file_path;
    context->current_output_path = prefix + L".new.dat";
    BrotliDecoderState* s = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!s) {
        safe_error(L"out of memory");
    }
    BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);
    if (_wfopen_s(&context->fin, context->current_input_path.c_str(), L"rb") != 0) {
        BrotliDecoderDestroyInstance(s);
        safe_error(file_path, L"not open");
    }

    if (_wfopen_s(&context->fout, context->current_output_path.c_str(), L"wb") != 0) {
        BrotliDecoderDestroyInstance(s);
        safe_error(file_path, L"not open");
    }

    DecompressFile(context, s);

    fclose(context->fin);
    fclose(context->fout);

    BrotliDecoderDestroyInstance(s);
}

size_t safe_convert(long long vl) {
    if (vl > ULONG_MAX) {
        safe_error(L"Memory Integer owerflow!!!", L"Allocate Buffer");
    }
    return (size_t)vl;
}

void write_blocks(long long count, FILE* sdat, FILE* img) {
    const int BUF_SIZE = 20 * 1024 * 1024;
    vector<char>buf(BUF_SIZE);

    while (count > BUF_SIZE)
    {
        safe_read(buf.data(), BUF_SIZE, sdat);
        safe_write(buf.data(), BUF_SIZE, img);
        count -= BUF_SIZE;
    }
    safe_read(buf.data(), safe_convert(count), sdat);
    safe_write(buf.data(), safe_convert(count), img);
}

void empty_blob(long long count, FILE* img) {
    const int BUF_SIZE = 8 * 1024 * 1024;
    vector<char>buf(BUF_SIZE);

    while (count > BUF_SIZE)
    {
        safe_write(buf.data(), BUF_SIZE, img);
        count -= BUF_SIZE;
    }
    safe_write(buf.data(), safe_convert(count), img);
}

void erase_zero(long long bstart, long long bend, FILE* img) {
    long long start = bstart * BLOCK_SIZE;
    long long end = bend * BLOCK_SIZE;
    _fseeki64(img, 0, SEEK_END);
    long long offset = _ftelli64(img);
    if (offset >= end)
        return;
    cout << "Zero Blob (";
    PrintBytes(end - offset);
    cout << ") -> offset: " << offset << endl;
    empty_blob(end - offset, img);
}

void copy_blocks(long long bstart, long long bend, FILE* sdat, FILE* img) {
    long long start = bstart * BLOCK_SIZE;
    long long end = bend * BLOCK_SIZE;
    _fseeki64(img, 0, SEEK_END);
    long long offset = _ftelli64(img);
    if (start > offset) {
        cout << "Writing Empty Blob (";
        PrintBytes(start - offset);
        cout << ") -> offset: " << offset << endl;
        empty_blob(start - offset, img);
    }
    _fseeki64(img, start, SEEK_SET);
    cout << "Copy " << bend - bstart << " Block -> position: " << bstart << endl;
    write_blocks(end - start, sdat, img);
}

void split(const string& str, const string& delim, vector<string>& parts) {
    size_t start, end = 0;
    while (end < str.size()) {
        start = end;
        while (start < str.size() && (delim.find(str[start]) != string::npos)) {
            start++;
        }
        end = start;
        while (end < str.size() && (delim.find(str[end]) == string::npos)) {
            end++;
        }
        if (end - start != 0) {
            parts.push_back(string(str, start, end - start));
        }
    }
}

bool DetectBrotli(wstring file_path) {
    size_t off = file_path.find_last_of(L'.');
    if (off == wstring::npos)
        return false;
    if (file_path.substr(off + 1).compare(L"br") == 0)
        return true;
    return false;
}

wstring MakePath(wstring dir, wstring file) {
    if (!dir.empty())
        return dir + L"\\" + file;
    return file;
}

wstring getPrefix(wstring file_path) {
    wstring path = L"";
    size_t poff = file_path.find_last_of(L"\\/");
    if (poff != wstring::npos) {
        path = file_path.substr(0, poff);
        file_path = file_path.substr(poff + 1);
    }


    size_t off = file_path.find_first_of(L'.');
    if (off == wstring::npos)
        return MakePath(path, file_path);
    return MakePath(path, file_path.substr(0, off));
}

int _tmain(int argc, _TCHAR* argv[]) {
    system("chcp 1251");
    setlocale(0, "Rus");
    cout<<"\n[sdat2img, support Brotli decompress]"<<endl<<endl;
    cout<<"   Re-written in C++   \n"<<endl;

    if (argc < 2) {
        cout<<"\nsdat2img - usage is: \n\n      sdat2img <system.new.dat>\n\n";
        exit(1);
    }

    wstring file_path = argv[1];

    wstring prefix = getPrefix(file_path);
    if (DetectBrotli(file_path)) {
        Context context;
        context.buffer = (uint8_t*)malloc(kFileBufferSize * 2);
        context.input = context.buffer;
        context.output = context.buffer + kFileBufferSize;
        DecomressBrotli(&context, file_path, prefix);
    }

    wstring NEW_DATA_FILE = prefix + L".new.dat";
    wstring TRANSFER_LIST_FILE = prefix + L".transfer.list";
    wstring OUTPUT_IMAGE_FILE = prefix + L".img";

    ifstream trans_list(TRANSFER_LIST_FILE);
    if (!trans_list.is_open()) {
        safe_error(TRANSFER_LIST_FILE, L" not found");
    }
    FILE* sdat = NULL;
    FILE*img = NULL;
    if (_wfopen_s(&sdat, NEW_DATA_FILE.c_str(), L"rb") != 0) {
        safe_error(NEW_DATA_FILE, L" not found");
    }

    if (_wfopen_s(&img, OUTPUT_IMAGE_FILE.c_str(), L"wb+") != 0) {
        safe_error(OUTPUT_IMAGE_FILE, L" not found");
    }

    //////////////////////////////
    string line;
    getline(trans_list, line);
    int version = stoi(line);
    getline(trans_list, line);
    long long blocks = stoll(line);
    cout << "[" << wchar_to_Cp1251(TRANSFER_LIST_FILE) << "]" << endl << "Info: Version " << version << ", Blocks " << blocks << ", BLOCK_SIZE " << BLOCK_SIZE << ", Data ";
    PrintBytes(blocks * BLOCK_SIZE);
    cout << endl << endl;

    if (version >= 2) {
        getline(trans_list, line);
        getline(trans_list, line);
    }
    long long i = 0;
    while (getline(trans_list, line)) {
        i++;
        string cmd, value;
        stringstream pr(line);
        pr >> cmd >> value;
        if (cmd.compare("new") != 0 && cmd.compare("erase") != 0 && cmd.compare("zero") != 0) {
            cout << "Skip line: " << line << endl;
            continue;
        }
        vector<string> params;
        split(value, ",", params);
        if (params.size() <= 0) {
            safe_error(TRANSFER_LIST_FILE, L"is corrupt, line: " + to_wstring(i));
        }
        int count = stoi(params[0]);
        if ((int)params.size() - 1 < count || count % 2 != 0) {
            safe_error(TRANSFER_LIST_FILE, L"is corrupt, line: " + to_wstring(i));
        }
        for (size_t s = 1; s < params.size(); s += 2) {
            long long st = stoll(params[s]);
            long long end = stoll(params[s + 1]);
            if(cmd.compare("new") == 0)
                copy_blocks(st, end, sdat, img);
            else
                erase_zero(st, end, img);
        }
    }
    //////////////////////////////

    fclose(sdat);
    fclose(img);
    trans_list.close();
    wprintf(L"Copyright (c) Umerov Artem, 2020\n");
    system("pause");
    return 0;
}