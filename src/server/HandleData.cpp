#include "../../inc/Webserv.hpp"
#include "../../inc/CgiHandler.hpp"

int	Webserv::SockReceiveData(void) {
	if (find(server_sockets_.begin(), server_sockets_.end(), curr_event_->ident) != server_sockets_.end()) {
		if (ConnectNewClient() == -1)
			return -1;
	}
	else if (find(server_sockets_.begin(), server_sockets_.end(), curr_event_->ident) == server_sockets_.end()) {
		buffer_.clear();
		buffer_.resize(BUFFER_SIZE);
		mapter_ = find_fd_.find(curr_event_->ident);
		for (wit_ = workers_.begin(); wit_ != workers_.end(); ++wit_)
			if (wit_->GetServerSocket() == mapter_->second)
				break ;
		ssize_t len = ReadData(curr_event_->ident, buffer_.data(), BUFFER_SIZE);
		if (len > 0) {
			if (StartReceiveData(len) == -1)
				return -1;
		}
		else if (len == 0) {
			std::cout << "Client " << curr_event_->ident << " disconnected." << std::endl;
			close (curr_event_->ident);
		}
		else
			return -1;
		if (event_data_->GetRequest().GetState() == READ_FINISH) {
			ReadFinish();
			CheckRequestError();
			if (this->event_data_->GetRequest().GetPath().find(".py") != std::string::npos || (this->event_data_->GetRequest().GetMethod() == "POST" && this->event_data_->GetRequest().GetPath().find(".bla") != std::string::npos)) {
				this->event_data_->GetResponse().SetResponseType(CGI);
				this->event_data_->GetCgiHandler().SetClientWriteIdent(curr_event_->ident);
				AddCgiEvent();
			}
			else {
				ChangeEvent(change_list_, curr_event_->ident, EVFILT_READ, EV_DISABLE, 0, 0, curr_event_->udata);
				ChangeEvent(change_list_, curr_event_->ident, EVFILT_WRITE, EV_ENABLE, 0, 0, curr_event_->udata);
			}
		}
	}
	return 0;
}

void	Webserv::SockSendData(void) {
	if (event_data_->GetResponse().GetStatusCode() != OK)
		event_data_->GetResponse().MakeStatusResponse(event_data_->GetResponse().GetStatusCode());
	else if (this->event_data_->GetResponse().GetResponseType() == GENERAL)
		MakeResponse();
	this->event_data_->GetResponse().SendResponse(curr_event_->ident);
	std::map<int, int>::iterator tmp_fd_iter = find_fd_.find(curr_event_->ident);
	find_fd_.erase(tmp_fd_iter);
	delete ((WorkerData *)curr_event_->udata);
	ChangeEvent(change_list_, curr_event_->ident, EVFILT_READ, EV_ENABLE, 0, 0, NULL);
	ChangeEvent(change_list_, curr_event_->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
	close (curr_event_->ident);
}

int	Webserv::StartReceiveData(int len) {
	buffer_.resize(len);
	if (event_data_->GetRequest().GetState() == HEADER_READ)
	{
		if (ReadHeader() == -1)
			return -1;
	}
	else if (event_data_->GetRequest().GetState() == BODY_READ)
		ReadBody();

	return 0;
}

int	Webserv::ReadHeader(void) {
	std::string temp_data(buffer_.begin(), buffer_.end());
	event_data_->GetRequest().AppendHeader(temp_data);
	event_data_->GetRequest().BodyAppendVec(buffer_);
	size_t pos = event_data_->GetRequest().GetHeaders().find("\r\n\r\n");
	if (pos == std::string::npos)
		return -1;
	else
	{
		std::string temp = event_data_->GetRequest().GetHeaders();
		event_data_->GetRequest().SetHeaders(event_data_->GetRequest().GetHeaders().substr(0, pos + 4));
		event_data_->GetRequest().RemoveCRLF();
		if (event_data_->GetRequest().GetHeaders().find("Content-Length") != std::string::npos)
		{
			size_t body_size = wit_->CheckContentLength(event_data_->GetRequest().GetHeaders());
			if (body_size <= event_data_->GetRequest().GetBody().size())
				event_data_->GetRequest().SetState(READ_FINISH);
			else
				event_data_->GetRequest().SetState(BODY_READ);
		}
		else if (event_data_->GetRequest().GetHeaders().find("Transfer-Encoding") != std::string::npos)
		{
			event_data_->GetRequest().AddRNRNOneTime();
			std::string temp = event_data_->GetRequest().GetBodyCharToStr();
			if (event_data_->GetRequest().Findrn0rn(temp) == 1)
				event_data_->GetRequest().SetState(READ_FINISH);
			else
				event_data_->GetRequest().SetState(BODY_READ);
		}
		else
			event_data_->GetRequest().SetState(READ_FINISH);
	}
	return 0;
}

void	Webserv::ReadBody(void) {
	event_data_->GetRequest().BodyAppendVec(buffer_);
	std::string temp(buffer_.begin(), buffer_.end());
	if (event_data_->GetRequest().GetHeaders().find("Content-Length") != std::string::npos)
	{
		size_t body_size = wit_->CheckContentLength(event_data_->GetRequest().GetHeaders());
		if (body_size <= event_data_->GetRequest().GetBody().size())	//본문을 다 읽으면
			event_data_->GetRequest().SetState(READ_FINISH);
	}
	else if (event_data_->GetRequest().GetHeaders().find("Transfer-Encoding") != std::string::npos)
	{
		if (event_data_->GetRequest().Findrn0rn(temp) == 1)
			event_data_->GetRequest().SetState(READ_FINISH);
	}
}

void	Webserv::ReadFinish(void) {
	wit_->RequestHeaderParse(event_data_->GetRequest());
	if (event_data_->GetRequest().GetHeaders().find("Transfer-Encoding") != std::string::npos)
	{
		event_data_->GetRequest().RemoveRNRNOneTime();
		wit_->ChunkBodyParse(event_data_->GetRequest(), event_data_->GetResponse());
	}
}

void    Webserv::AddCgiEvent(void) {
	this->event_data_->GetCgiHandler().ExecuteChildProcess(this->event_data_->GetRequest());
	this->SetCgiEvent();
}

void    Webserv::CheckRequestError(void) {
	for (int i = 0; i < wit_->GetLocations().size(); i++)
	{
		if (event_data_->GetRequest().GetPath().find(wit_->GetLocations()[i].GetUri()) != std::string::npos) {
			if (wit_->GetLocations()[i].GetUri().length() != 1)
			{
				location_idx_ = i;
				break ;
			}
			location_idx_ = i;
		}
	}

	std::string method = event_data_->GetRequest().GetMethod();
	std::map<int, bool> limit_excepts = wit_->GetLocations()[location_idx_].GetLimitExcepts();

	std::string allowed = "";
	if (limit_excepts[METHOD_GET])
		allowed += "GET, ";
	if (limit_excepts[METHOD_POST])
		allowed += "POST, ";
	if (limit_excepts[METHOD_DELETE])
		allowed += "DELETE, ";
	allowed.pop_back();
	allowed.pop_back();

	if (!(method == "GET" || method == "POST" || method == "DELETE"))
		return event_data_->GetResponse().SetStatusCode(NOT_IMPLEMENTED);
	else if (!(method == "GET" &&  limit_excepts[METHOD_GET]
           || method == "POST" && limit_excepts[METHOD_POST]
           || method == "DELETE" && limit_excepts[METHOD_DELETE])) {
		event_data_->GetResponse().SetAllow(allowed);
		return event_data_->GetResponse().SetStatusCode(METHOD_NOT_ALLOWED);
	}

	if (event_data_->GetRequest().GetScheme() != "HTTP/1.1")
		return event_data_->GetResponse().SetStatusCode(HTTP_VERSION_NOT_SUPPORTED);

	if (event_data_->GetRequest().GetContentLength() > wit_->GetLocations()[location_idx_].GetClientMaxBodySizeLocation())
		return event_data_->GetResponse().SetStatusCode(PAYLOAD_TOO_LARGE);
	/*
	 * 요청을 다 읽은 시점에서 예외처리 필요
	 * ㅁ 경로가 올바른지?
	 * ㅁ 파일 및 폴더 열 수 있는지?
	 * V 지원하지 않는 요청 메서드인지?
	 * V http 버전이 잘못되었는지?
	 * V content length 관련 : client_max_body_size를 넘는지?
	 * -> Request에 집어넣은 헤더 모두 검사한다고 생각하면 될 듯
	 */
}

void	Webserv::SetCgiEvent(void) {
	event_data_->SetEventType(CGIEVENT);
	ChangeEvent(change_list_, this->event_data_->GetCgiHandler().GetReadFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, event_data_);
	ChangeEvent(change_list_, this->event_data_->GetCgiHandler().GetWriteFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, event_data_);

	fcntl(this->event_data_->GetCgiHandler().GetReadFd(), F_SETFL, O_NONBLOCK, FD_CLOEXEC);
	fcntl(this->event_data_->GetCgiHandler().GetWriteFd(), F_SETFL, O_NONBLOCK, FD_CLOEXEC);
}

/*
 * 응답 헤더의 흐름
 * - Response 객체를 만드는 시점에서 헤더 값 default로 두기
 * - request 파싱이 끝난 시점에서 예외처리 + 헤더값 일부 변경하기
 * - 또 처리하는 과정에서 헤더 값 수정이 필요한 경우 setter로 변경하기
 */
void    Webserv::MakeResponse() {
	std::string method = event_data_->GetRequest().GetMethod();
    if (method == "GET")
        event_data_->GetResponse().HandleGet(event_data_->GetRequest(), wit_->GetLocations()[location_idx_].GetIndex());
    else if (method == "POST")
        event_data_->GetResponse().HandlePost(event_data_->GetRequest());
    // else if (method == "PUT" && limit_excepts[METHOD_PUT])
    //     this->event_data_->response.HandlePost(event_data_->request);
    else if (method == "DELETE" )
        event_data_->GetResponse().HandleDelete(event_data_->GetRequest());
}