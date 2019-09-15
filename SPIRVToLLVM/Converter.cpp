#include "Converter.h"

#include "LLVMSPIRVLib.h"

#include <llvm/IR/Verifier.h>

#include <string>

#define FATAL_ERROR() { __debugbreak(); abort(); }

struct membuf : std::streambuf
{
	membuf(char const* base, size_t size)
	{
		auto p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
};

struct imemstream final : virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
		: membuf(base, size), std::istream(static_cast<std::streambuf*>(this))
	{
	}
};

__declspec(dllexport) void ConvertSpirv(const std::vector<uint32_t>& spirv, spv::ExecutionModel executionModel)
{
	llvm::LLVMContext llvmContext;
	llvm::Module* llvmModule;
	std::string error;

	imemstream stream{reinterpret_cast<const char*>(spirv.data()), spirv.size() * 4};

	if (!readSpirv(llvmContext, SPIRV::TranslatorOpts{SPIRV::VersionNumber::MaximumVersion, executionModel}, stream, llvmModule, error))
	{
		FATAL_ERROR();
	}

	// LLVM_DEBUG(dbgs() << "Converted LLVM module:\n" << *M);

	llvm::raw_string_ostream ErrorOS(error);
	if (!verifyModule(*llvmModule, &ErrorOS))
	{
		FATAL_ERROR();
	}

	std::string dump;
	llvm::raw_string_ostream DumpStream(dump);
	llvmModule->print(DumpStream, nullptr);

	// WriteBitcodeToFile();
	
	// if (OutputFile.empty()) {
	// 	if (InputFile == "-")
	// 		OutputFile = "-";
	// 	else
	// 		OutputFile = removeExt(InputFile) + kExt::LLVMBinary;
	// }
	//
	// std::error_code EC;
	// ToolOutputFile Out(OutputFile.c_str(), EC, sys::fs::F_None);
	// if (EC) {
	// 	errs() << "Fails to open output file: " << EC.message();
	// 	return -1;
	// }
	//
	// WriteBitcodeToFile(*M, Out.os());
	// Out.keep();
	// delete M;

	FATAL_ERROR();
}
