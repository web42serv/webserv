#include "Request.hpp"

Request::Request()
{

}

Request::~Request()
{

}

void	Request::setMethod(std::string method)
{
	this->httpMethod = method;
}

void	Request::setPath(std::string path)
{
	this->path = path;
}

void	Request::setScheme(std::string scheme)
{
	this->scheme = scheme;
}

void	Request::pushBackHost(std::string host)
{
	this->host.push_back(host);
}

void	Request::setConnection(std::string connection)
{
	this->connection = connection;
}

void	Request::setContentLength(std::string contentLength)
{
	this->contentLength = contentLength;
}

void	Request::setBody(std::string body)
{
	this->body = body;
}

std::string	Request::getMethod()
{
	return (this->httpMethod);
}

std::string	Request::getPath()
{
	return (this->path);
}

std::string	Request::getScheme()
{
	return (this->scheme);
}

std::vector<std::string>	Request::getHost()
{
	return (this->host);
}

std::string	Request::getConnection()
{
	return (this->connection);
}

std::string	Request::getContentLength()
{
	return (this->contentLength);
}

std::string Request::getBody()
{
	return (this->body);
}