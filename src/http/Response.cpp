#include "../../inc/Response.hpp"


Response::Response()
{
	this->type = GENERAL;
	this->statusCode = OK;
	this->connection = "keep-alive";
	this->contentType = "text/html";
	this->httpVersion = "HTTP/1.1";
	this->location = "";
}

Response::~Response()
{

}

Response& Response::operator=(const Response& response)
{
	this->statusCode = response.statusCode;
	this->connection = response.connection;
	this->contentType = response.contentType;
	this->httpVersion = response.httpVersion;
	this->location = response.location;
	return *this;
}

std::string Response::getStatusMessage(int code)
{
	switch (code)
	{
		case OK:
			return "OK";
		case CREATED:
			return "Created";
		case ACCEPTED:
			return "Accepted";
		case NO_CONTENT:
			return "No Content";
		case BAD_REQUEST:
			return "Bad Request";
		case UNAUTHORIZED:
			return "Unauthorized";
		case FORBIDDEN:
			return "Forbidden";
		case NOT_FOUND:
			return "Not Found";
		case INTERNAL_SERVER_ERROR:
			return "Internal Server Error";
		case NOT_IMPLEMENTED:
			return "Not Implemented";
		case SERVICE_UNAVAILABLE:
			return "Service Unavailable";
		case CONTENT_TOO_LARGE:
			return "Content Too Large";
		default:
			return "Unknown";
	}
}

void Response::readFileToBody(const std::string &path)
{
	std::ifstream fin;
	fin.open(path.c_str());
	if (fin.fail())
		std::cerr << "file open error" << std::endl;
	std::string line;
	while (getline(fin, line))
	{
		line += "\r\n";
		for (int i = 0; i < line.length(); i++)
			this->body.push_back(line[i]);
	}
	fin.close();
}

void Response::generateBody_AutoIndexing(const Request &request)
{
	this->readFileToBody(AUTO_INDEX_HTML_PATH); // 템플릿 전부 읽기

	std::vector<char> v = this->body;
	std::string string_body(v.begin(), v.end());
	size_t toInsert;

	//title 지정
	toInsert = string_body.find("</title>");
	string_body.insert(toInsert, "Index of " + request.getPath());

	//head 지정
	toInsert = string_body.find("</h1>");
	string_body.insert(toInsert, "Index of " + request.getPath());

	// files 이름 지정
	std::string requestPath = request.getPath();
	if (requestPath.back() != '/') // request path가 '/'로 끝나지 않는 directory일 때 버그 방지
		requestPath.push_back('/');
	std::vector<std::string> fileNames = getFilesInDirectory(request.getFullPath());
	for (size_t i = 0; i < fileNames.size(); i++)
	{
		toInsert = string_body.rfind("<hr>");
		string_body.insert(toInsert,std::string() +
		                            "\t<pre>" + "<a href=\"" + requestPath
		                            + fileNames[i] + "\">" + fileNames[i] + "</a></pre>\r\n");
	}
	std::vector<char> v1(string_body.begin(), string_body.end());
	this->body = v1;
}

int Response::checkPath(const std::string path)
{
	struct stat buf;

	if (stat(path.c_str(), &buf) == -1) // 해당 경로에 파일이 존재 안하면 404Page
		return 0;
	else if (S_ISDIR(buf.st_mode)) // 경로일 때
	{
		if (path.back() == '/')
			return 1;
		else
			return 2;
	}
	return 3;
}

std::vector<std::string> Response::getFilesInDirectory(const std::string &dirPath)
{
	DIR *dir_info;
	struct dirent *dir_entry;
	std::vector<std::string> ret;

	if ((dir_info = opendir(dirPath.c_str())) == NULL)
		std::cerr << "opendir error" << std::endl;
	while ((dir_entry = readdir(dir_info)))
	{
		if (std::strcmp(dir_entry->d_name, ".") == 0)
			continue;
		if (dir_entry->d_type == DT_DIR) // directory 일 경우 뒤에 / 추가.
			ret.push_back(std::string(dir_entry->d_name) + "/");
		else
			ret.push_back(dir_entry->d_name);
	}
	closedir(dir_info);

	return ret;
}

// void Response::handleBodySizeLimit()
// {
// 	this->statusCode = CONTENT_TOO_LARGE;
// 	this->connection = "close";
// 	this->readFileToBody(ERROR_PAGE_413_PATH);
// 	this->contentLength = this->body.size();
// 	this->contentType = "text/html";
// }

// void Response::handleBadRequest()
// {
// 	this->httpVersion = "HTTP/1.1";
// 	this->statusCode = NOT_FOUND;
// 	this->connection = "close";
// 	this->readFileToBody(ERROR_PAGE_404_PATH);
// 	this->contentLength = this->body.size();
// 	this->contentType = "text/html";
// }

void Response::handleGET(const Request &request, const std::string index)
{
	std::string final_path = request.getFullPath();
	int check_res = this->checkPath(final_path);
	if (check_res == 1) // autoindex 해야하는 상황
	{
		this->generateBody_AutoIndexing(request);
		return;
	}
	else if (check_res == 2) // 파일이 없거나 디렉토리인데 구조 이상한 경우
	{
		final_path += "/";
		int last_slash = final_path.find_last_of("/");
		std::string last_dir = final_path.substr(0, last_slash + 1);
		final_path = last_dir + index;
		int second_check = this->checkPath(final_path);
		if (second_check == 0)
		{
			/* 바로 404 응답 보내는 함수 필요 */
			this->statusCode = NOT_FOUND;
			final_path = ERROR_PAGE_404_PATH;
		}
	}
	else if (check_res == 0) // youpi.bad_extension 파일조차 없거나 원래부터 유효하지 않은 파일인 경우
	{
		/* 바로 404 응답 보내는 함수 필요 */
		this->statusCode = NOT_FOUND;
		final_path = ERROR_PAGE_404_PATH;
	}
	this->readFileToBody(final_path);
}

void Response::handlePOST(const Request &request) {
	DIR *dir_info;

	/* 405 응답 보내는 함수 필요 */
	if ((dir_info = opendir(request.getPath().c_str())) != NULL) {
		this->setHttpVersion("HTTP/1.1");
		this->setStatusCode(405);
		this->contentType = request.getContentType();
		this->connection = "Close";
		closedir(dir_info);
		return ;
	}
}

void Response::handlePUT(const Request &request) {}

void Response::handleDELETE(const Request &request) {
	std::string path = request.getFullPath();
//	try
//	{
		std::string body = deleteCheck(path);
		std::vector<char> v(body.begin(), body.end());
		this->body = v;
//	}
//	catch (std::runtime_error &e)
//	{
//		std::cout << e.what() << std::endl;
//		this->statusCode = NOT_FOUND;
//		std::string failed = "delete failed\n";
//		std::vector<char> v(failed.begin(), failed.end());
//		this->body = v;
//		return ;
//	}
}

void Response::setStatusCode(int data)
{
	this->statusCode = data;
}

void    Response::SendResponse(int fd) {
	std::string toSend;

	// 임시 하드코딩
	toSend += this->httpVersion;
	toSend += " " + std::to_string(this->statusCode);
	toSend += " " + this->getStatusMessage(this->statusCode);
	toSend += "\r\n";

	if (!this->contentType.empty())
		toSend += "Content-Type: " + this->contentType + "\r\n";
	if (!this->body.empty())
		toSend += "Content-Length: " + std::to_string(this->body.size()) + "\r\n";
	if (!this->connection.empty())
		toSend += "Connection: " + this->connection + "\r\n";
	toSend += "\r\n";

	std::string tmp(this->body.begin(), this->body.end());

	toSend += tmp;
	// write error 처리 필요
	if (send(fd, toSend.c_str(), toSend.size(), 0) == -1)
		std::cerr << "write error" << std::endl;
}

std::string Response::deleteCheck(std::string path) const
{
	if (access(path.c_str(), F_OK) == 0)
	{
		if (access(path.c_str(), W_OK) == 0)
		{
			if (unlink(path.c_str()) == 0)
				return path + " deleted\n";
			return "unlink error";
		}
		return "permission error";
	}
	else
		return "404 not found";
}

void    Response::setHttpVersion(std::string version) {
	this->httpVersion = version;
}

void    Response::pushBackBody(char c)
{
	this->body.push_back(c);
}

void    Response::printBody() const
{
	std::cerr << "-- print body --" << std::endl;
	for(int i=0; i<this->body.size(); i++)
		std::cerr << this->body[i];
	std::cerr << "-- finish --" << std::endl;
}

ResponseType Response::getResponseType() const
{
	return (this->type);
}

void    Response::setResponseType(ResponseType type)
{
	this->type = type;
}