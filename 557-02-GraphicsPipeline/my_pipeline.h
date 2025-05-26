#ifndef __MY_PIPELINE_H__
#define __MY_PIPELINE_H__

#include <string>
#include <vector>


class MyPipeline
{
public:
	MyPipeline(const std::string& vertFilepath, const std::string& fragFilepath);

private:
	static std::vector<char> _readFile(const std::string& filename);

	void _createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath);
};

#endif

