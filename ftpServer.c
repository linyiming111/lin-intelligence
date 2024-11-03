#include <stdio.h>
#include"ftpServer.h"
char g_recvBuf[10240];// �����Զ���ʼ��Ϊ��
int g_fileSize;//��ȡ�ļ���С
char* g_fileBuf;
#pragma warning(disable : 4996)

int main()
{
	initSocket();
	listenToClient();
	closeSocket();
	return 0;
}

/*

�ͻ������������ļ�->���ļ������͸�������
���������ܿͻ��˷��͵��ļ���->�����ļ������ҵ��ļ������ļ���С�����ͻ���
�ͻ��˽��ܵ���С����ʼ׼�������ڴ棬���߷�����׼������
���������ܿ�ʼ����ָ��->��ʼ��������
��ʼ���ܣ�������->������ɣ����߷������������
�ر�����
*/


//��ʼ��socket��
bool initSocket()
{
	WSADATA wsadata;
	//makeword�������Э��汾
	//ֻ����һ�γɹ���WSAStartup()����֮����ܵ��ý�һ����Windows Sockets API������
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		printf("WSAStartup faild :%d\n", WSAGetLastError());
        return FALSE;
	}
	return TRUE;

}
//�ر�socket��
bool closeSocket()
{
	if (0 != WSACleanup())
	{
		printf("WSACleaup faild:%d\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}
//�����ͻ�������
void listenToClient()
{
	printf("listen to client....\n");
	//����server socket�׽��� ��ַ���˿ں�
	//INVALID_SOKET SOCKET_ERROR
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		err("socket");
		return;
	}
	//��socket��ip�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;//����ʹ�����socketָ����һ��
	serAddr.sin_port = htons(SPORT);
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY;//����������������
	//���׽���
	if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		err("bind");
		return;
	}
	//�����ͻ�������
	if (0 != listen(serfd, 10))
	{
		err("listen");
		return;
	}
	//�пͻ������ӣ��ͽ���
	struct sockaddr_in cliAddr;
	int len = sizeof(cliAddr);
	SOCKET clifd = accept(serfd, (struct sockaddr*)&cliAddr, &len);
	if (INVALID_SOCKET == clifd)
	{
		err("accept");
		return;
	}
	printf("new client connect success....\n");
	//��ʼ������Ϣ
	while (processMsg(clifd)){}
}
//������Ϣ����
bool processMsg(SOCKET clifd)
{
	int nRes = recv(clifd, g_recvBuf, 10240, 0);
	//�ɹ�������Ϣ�����ؽ��ܵ����ֽ���������ʧ�ܷ���0
	if (nRes <=0)
	{
		printf("�ͻ������ߡ�������%d\n", WSAGetLastError());
		return false;
	}
	//�����ܵ��Ļ�����ϢǿתΪ�ṹ�����ͣ�����֮�����Ϣ�ж�
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
	//���ڲ��޷�����ṹ�壬ֻ�������涨�壬������Ϣ�ķ���
	struct MsgHeader exitmsg;
		switch (msg->msgID)
		{
		  case MSG_FILENAME:
			  printf("Ҫ���͵��ļ��ǣ�%s\n", msg->fileInfo.fileName);
			  readFile(clifd, msg);//�����ļ�
			  break;
		  case MSG_SENDFILE:
			  sendFile(clifd, msg);//�����ļ�
			  break;
		  case MSG_SUCCESSED:
			  exitmsg.msgID = MSG_SUCCESSED;
			  if (SOCKET_ERROR == send(clifd, (char*)&exitmsg, sizeof(struct MsgHeader), 0))
			  {
				printf("send faild :%d\n", WSAGetLastError());
				return false;
			  }
			  printf("�ͻ����ѳɹ����ܣ�~\n");
			  break;
		  case ENDSEND:
			  printf("�ͻ������ߣ�bye~bye~\n");
			  closesocket(clifd);
			  return FALSE;
			  break;
		  case CHAT_ING:
			  printf("recv:%s\n", msg->CHAT.chatbuf);
			  memset(msg, 0, sizeof(msg));//��ջ����ֹ���
			  HANDLE hThread[2];//�����߳̾��
			  unsigned threadID[2];

			  hThread[0] = (HANDLE)_beginthreadex(NULL, 0, p_send, &clifd, 0, &threadID[0]);//���߳�send;
			  hThread[1] = (HANDLE)_beginthreadex(NULL, 0, p_recv, &clifd, 0, &threadID[1]);//���߳�recv;

			  WaitForSingleObject(hThread[0], INFINITE);
			  WaitForSingleObject(hThread[1], INFINITE);
			  CloseHandle(hThread[0]);
			  CloseHandle(hThread[1]);
			  break;
		  case CHAT_END:
			  printf("�ͻ��˲���������\n");
			  closesocket(clifd);
			  memset(msg, 0, sizeof(msg));
			  return false;
			  break;
		}
		return true;
	}
//���̷߳��ͷ�����
unsigned __stdcall p_send(void* pfd) {
	SOCKET fd;
	fd = *((SOCKET*)pfd);
	struct MsgHeader p_exitmsg;
	while (true)
	{
		printf("send>");
		gets_s(p_exitmsg.CHAT.chatbuf, PACKET_SIZE);
		//����˶Խ��ܵ�����Ϣ���д������close��ر�����
		if (strlen(p_exitmsg.CHAT.chatbuf) > 200)
		{
			printf("the buffer is too big!\n");
			memset(&p_exitmsg, 0, sizeof(p_exitmsg));
			continue;
		}
		if (strcmp(p_exitmsg.CHAT.chatbuf, "close") == 0)
		{
			p_exitmsg.msgID = CHAT_END;
			if (SOCKET_ERROR == send(fd, (char*)&p_exitmsg, sizeof(struct MsgHeader), 0))
			{
				err("send");
			}
			printf("��ֹ�Ự��\n");
			closesocket(fd);
			return false;
		}
		p_exitmsg.msgID = CHAT_ING;
		if (SOCKET_ERROR == send(fd, (char*)&p_exitmsg, sizeof(struct MsgHeader), 0))
		{
			printf("client had closed!\n");
			return false;
		}
		memset(&p_exitmsg, 0, sizeof(p_exitmsg));
	}
}
//���߳̽��ܺ���
unsigned __stdcall p_recv(void* pfd) {
	SOCKET fd;
	fd = *((SOCKET*)pfd);
	struct MsgHeader p_exitmsg;//���ڷ�����Ϣ
	while (true) {
		int nRes = recv(fd, g_recvBuf,10240, 0);
		if (0 < nRes) {
			struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
			switch (msg->msgID)
			{
			case MSG_FILENAME:
				printf("Ҫ���͵��ļ��ǣ�%s\n", msg->fileInfo.fileName);
				readFile(fd, msg);//�����ļ�
				break;
			case MSG_SENDFILE:
				sendFile(fd, msg);//�����ļ�
				break;
			case MSG_SUCCESSED:
				p_exitmsg.msgID = MSG_SUCCESSED;
				if (SOCKET_ERROR == send(fd, (char*)&p_exitmsg, sizeof(struct MsgHeader), 0))
				{
					printf("send faild :%d\n", WSAGetLastError());
					return false;
				}
				printf("�ͻ����ѳɹ����ܣ�~\n");
				break;
			case ENDSEND:
				printf("�ͻ������ߣ�bye~bye~\n");
				closesocket(fd);
				return FALSE;
				break;
			case CHAT_END:
				return false;
				break;
			case CHAT_ING:
				printf("recv:%s\n", msg->CHAT.chatbuf);
				memset(msg, 0, sizeof(msg));//��ջ����ֹ���
				break;
			}
		}
	}
}
//���ļ�����ȡ�ļ���С
bool readFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	FILE *pread = fopen(pmsg->fileInfo.fileName, "rb");
	//�Ҳ����͸��߿ͻ��˷���ʧ��
	if (pread == NULL)
	{
		printf("�Ҳ���[%s]�ļ�..\n", pmsg->fileInfo.fileName);
		struct MsgHeader msg;
		msg.msgID = MSG_OPENFILE_FAILD;
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("send faild:%d\n", WSAGetLastError());
		}
		return FALSE;
	}
	//�ҵ��ļ��󣬻�ȡ�ļ���С
	fseek(pread, 0, SEEK_END);
	g_fileSize =ftell(pread);//���� ftell ���ڵõ��ļ�λ��ָ�뵱ǰλ��������ļ��׵�ƫ���ֽ���
	fseek(pread, 0, SEEK_SET);//��ָ���Ƶ��ļ���ʼ�ط���׼����ʼ��
	struct MsgHeader msg;
	msg.msgID = MSG_FILESIZE;
	//���ļ���С�����ͻ���
	msg.fileInfo.fileSize = g_fileSize;
	char tfname[200] = {0}, text[100];
	_splitpath(pmsg->fileInfo.fileName, NULL, NULL,tfname, text);//��ȡ�ļ������ֺ�����
	strcat(tfname,text);
	//���ļ��������ͻ���
	strcpy(msg.fileInfo.fileName,tfname);
	send(clifd,(char*)&msg, sizeof(struct MsgHeader), 0);
	//��ȡ�ļ�����
	//�����ڴ�ռ�
	g_fileBuf = calloc(g_fileSize+1, sizeof(char));
	if (g_fileBuf == NULL)
	{
		printf("�ڴ治�㣬������\n");
		return false;
	}
	fread(g_fileBuf ,sizeof(char),g_fileSize,pread);
	g_fileBuf[g_fileSize] = '\0';
	fclose(pread);
	return true;
}
//���ļ�
bool sendFile(SOCKET clifd, struct MsgHeader* pmsg)
{
	//���߿ͻ���׼�������ļ���
	//����ļ��ĳ��ȴ���ÿ�����ݰ��ܴ��͵Ĵ�С(packet_size)��ô�ͷֿ�
	struct MsgHeader msg;
	msg.msgID = MSG_READY_READ;
	
	for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE)
	{
		msg.packet.nStart = i;
		//�ж��Ƿ�Ϊ���һ��������ȡ���һ�����ĳ���
		if (i + PACKET_SIZE+1 >g_fileSize)
		{
			msg.packet.nsize = g_fileSize - i;
		}
		else
		{
			msg.packet.nsize = PACKET_SIZE;
		}
		memcpy(msg.packet.buf, g_fileBuf + msg.packet.nStart, msg.packet.nsize);
		if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			printf("�ļ�����ʧ��%d\n",WSAGetLastError());
			return false;
		}	
	}
	
	return TRUE;
}
