#include <stdio.h>
#include"ftpClient.h"
#include"string.h"
#pragma warning(disable : 4996)
char g_recvBuf[10240];       //������Ϣ�Ļ�����///!!!!!!
char *g_fileBuf;        //�洢�ļ�����
int g_fileSize;         //�յ����ļ���С
char g_fileName[256];        //������������͹������ļ���

int main()
{
	initSocket();
	connectToHost();
	closeSocket();
	return 0;
}
//��ʼ��socket��
bool initSocket()
{
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		err("WSAStartup");
		return FALSE;
	}
	return TRUE;

}
//�ر�socket��
bool closeSocket()
{
	if (0 != WSACleanup())//WSACleanup������ֹ��Windows Sockets�������߳��ϵĲ���.
	{
		err("WSACleaup");
		return FALSE;
	}
	return TRUE;

}
//�����ͻ�������
void connectToHost()
{
	//����server socket�׽��� ��ַ���˿ں�
	//INVALID_SOKET SOCKET_ERROR
	SOCKET  serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		printf("socket faild :%d\n", WSAGetLastError());
		return;
	}
	//��socket��ip�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;//����ʹ�����socketָ����һ��
	serAddr.sin_port = htons(SPORT);
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//����������������
	//���ӵ�������
	if (0 != connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
	{
		printf("connnect faild :%d\n", WSAGetLastError());
		return;
	}
	printf("conect success!");

	//��ʼ������Ϣ,ѡ�������ҹ��ܻ����ļ����书�ܣ�
	char ch;
	while (true) {
		printf("chat or sendfile?\nif chat press a,if sendfile press b��else you will exit this systm!\n");
		printf("***********************************\n");
		printf("     a: chat\n");
		printf("     b: sendfile\n");
		printf("     c:exit this window\n");
		printf("***********************************\n");
		printf("Enter your choice (a or b or c): ");
		//���������Ϣ����ȷ����������
		do{
			ch = (char)_getch();
		} while (ch != 'a' && ch != 'b' && ch != 'c');

		system("cls"); //�������̨��ʾ����Ϣ
		
		//����������
		if (ch == 'a')
		{
			printf("welcome to yy's chatroom!\n");
			chatroom(serfd);
			break;
		}
		//�����ļ�����
		if (ch == 'b') {
			printf("welcome to download file!\n");
			//��ȡҪ�����ļ�����
			downloadFileName(serfd);//�ȷ����ļ�����������
			while (processMsg(serfd)){}
		}
		//�˳�ϵͳ
		if (ch == 'c') {
			printf("see you next time!\n");
			closesocket(serfd);
			return false;
		}
		printf("\nPress Any Key To Continue:");
		_getch();
		system("cls");
	}
	return true;
}



//������Ϣ
bool processMsg(SOCKET serfd)
{
	recv(serfd, g_recvBuf, 10240, 0);
	struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;//�����ܵ�����ϢǿתΪ����ʽ
	switch (msg->msgID)
	{
	case MSG_OPENFILE_FAILD:
		printf("send file failed ,please try again\n");
		downloadFileName(serfd);
		break;
	case MSG_FILESIZE:
		readyread(serfd, msg);
		break;
	case MSG_READY_READ:
		writeFile(serfd, msg);
		break;
	case MSG_SUCCESSED:
		printf("send file success!\n");
		//����ѡ���
		char key;
		while (true)
		{
			printf("if you want to continue to download,print 1,if you want to chat,print 2,else press another key:\n ");
			printf("***********************************\n");
			printf("     1: download\n");
			printf("     2: chat\n");
			printf("     3:exit this window\n");
			printf("***********************************\n");
			printf("Enter your choice (1 or 2 or 3): ");
			do {
				key = (char)_getch();
			} while (key != '1'&& key !='2' && key != '3');

			system("cls"); //�������̨��ʾ����Ϣ
			if (key == '1') {
				downloadFileName(serfd);
				while (processMsg(serfd)){}
			}
			if (key =='2') {
				struct MsgHeader Chat;
				Chat.msgID = CHAT_ING;
				printf("welcome to yy's chatroom!\n");
				chatroom(serfd);
				break;	
			}
			if (key =='3') {
				struct MsgHeader file;
				file.msgID = ENDSEND;
				if (SOCKET_ERROR == send(serfd, (char*)&file, sizeof(struct MsgHeader), 0))
				{
					err("send");
					return;
				}
				printf("�ر�ϵͳ����ӭ�´��ټ�~\n");
				closesocket(serfd);
				return false;
			}
		}
		break;
	}
	return TRUE;
}
//�����ļ���	
void downloadFileName(SOCKET serfd)
{
	char fileName[1024];
	printf("������Ҫ���ص��ļ�����");
	gets_s(fileName,1024);
	struct MsgHeader file;
	file.msgID = MSG_FILENAME;
	strcpy(file.fileInfo.fileName,fileName);
	if (SOCKET_ERROR == send(serfd, (char*)&file, sizeof(struct MsgHeader), 0))
	{
		err("send");
		return;
	}
}
//׼����
void readyread(SOCKET serfd, struct MsgHeader*pmsg)//�ú������������ڴ�
{
	strcpy(g_fileName,pmsg->fileInfo.fileName);
	//׼���ڴ� pmsg->fileInfo.fileSize
	g_fileSize = pmsg->fileInfo.fileSize;
	//����ռ�
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL)//�ڴ�����ʧ��
	{
		printf("�ڴ治�㣬������\n");	
	}
	else
	{
		struct MsgHeader msg;
		msg.msgID = MSG_SENDFILE;
		if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			err("send");
			return;
		}
	}
}
//д�ļ�
bool writeFile(SOCKET serfd, struct MsgHeader* pmsg)
{
	//���g_fileBUfΪ�գ�������ڴ�ʧ��
	if (g_fileBuf == NULL)
	{
		return FALSE;
	}
	//��ָ���Ƶ���ʼ����λ��
	int nStart = pmsg->packet.nStart;
	//��ȡ������
	int nsize = pmsg->packet.nsize;
	//ƴ��
	memcpy(g_fileBuf+nStart,pmsg->packet.buf, nsize);
	//�ж��ļ��Ƿ��������������Ƿ��������ݣ����ܵ����һ����ʱ��ʼд��
	if(nStart + nsize >= g_fileSize)
	{
		FILE* pwrite = fopen(g_fileName, "wb");
		if (pwrite == NULL)
		{
			printf("write file error..\n");
			return false;
		}
		fwrite(g_fileBuf, sizeof(char), g_fileSize, pwrite);
		//д���ͷŻ��棬�رն�ָ��
		fclose(pwrite);
		free(g_fileBuf);
		g_fileBuf = NULL;
		//����д��ɹ���Ϣ
		struct MsgHeader msg;
		msg.msgID = MSG_SUCCESSED;
		if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))
		{
			err("send");
			return;
		}
	}
	return true;
}
//������
 bool chatroom(SOCKET serfd)
{
	 HANDLE hThread[2];//���������߳�
	 unsigned threadID[2];
	 hThread[0] = (HANDLE)_beginthreadex(NULL, 0, p_send, &serfd, 0, &threadID[0]);//���߳�p_send;
	 hThread[1] = (HANDLE)_beginthreadex(NULL, 0, p_recv, &serfd, 0, &threadID[1]);//���߳�p_recv;
	 WaitForSingleObject(hThread[0], INFINITE);//�ȴ��߳̽���
	 WaitForSingleObject(hThread[1], INFINITE);
	 CloseHandle(hThread[0]);//�ر��߳�
	 CloseHandle(hThread[1]);
	
}
 //���̷߳��ͺ���
 unsigned __stdcall p_send(void* pfd) {
	 SOCKET fd;
	 fd = *((SOCKET*)pfd);
	 struct MsgHeader Chat3;//���ڷ�����Ϣ
	 int flag = 0;//���ڱ���Ƿ��˳�
	 while (flag==0) {
		 printf("send>");
		 gets_s(Chat3.CHAT.chatbuf, PACKET_SIZE);
		 //��������������close���͹ر�
		 if (strlen(Chat3.CHAT.chatbuf) > 200)
		 {
			 printf("the buffer is too big!\n");
			 memset(&Chat3, 0, sizeof(Chat3));
			 continue;
		 }
		 if (strcmp(Chat3.CHAT.chatbuf, "close") == 0)
		 {
			 Chat3.msgID = CHAT_END;
			 send(fd, (char*)&Chat3, sizeof(struct MsgHeader), 0);
			 memset(&Chat3, 0, sizeof(Chat3.CHAT.chatbuf));
			 printf("exit chatroom,bye~bye~\n");
			 flag = 1;//��־����
			 closesocket(fd);
			 return false; 
		 }
		 //�������sendfile���ͷ����ļ�
		 if (strcmp(Chat3.CHAT.chatbuf, "sendfile") == 0)
		 {
			 downloadFileName(fd);
		 }
		Chat3.msgID = CHAT_ING;
		if (SOCKET_ERROR == send(fd, (char*)&Chat3, sizeof(struct MsgHeader), 0))
		{
			printf("server had closed!\n");
			return false;
		}
		memset(&Chat3, 0, sizeof(Chat3.CHAT.chatbuf));
	 }
 }
 //���߳̽��ܺ���
 unsigned __stdcall p_recv(void* pfd) {
	 int flag = 0;//���ڱ���˳�,Ϊ0���˳���Ϊ1�˳�
	 while (flag==0) {
		 SOCKET fd;
		 fd = *((SOCKET*)pfd);
		 int nRes = recv(fd, g_recvBuf, 10240, 0);
		 if (0 < nRes) {
			 struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
			 //��Ϊ������������漰��������Ϣ���ܣ�����Ҫ����Ϣ�����жϣ��ж��Ƿ�Ϊ�ļ�����������Ϣ
			 switch (msg->msgID)
			 {
			 case MSG_OPENFILE_FAILD:
				 printf("send file failed ,please try again\n");
				 downloadFileName(fd);
				 break;
			 case MSG_FILESIZE:
				 readyread(fd, msg);
				 break;
			 case MSG_READY_READ:
				 writeFile(fd, msg);
				 break;
			 case MSG_SUCCESSED:
				 printf("send file success!\n");
				 //����ѡ���
				 break;
			 case CHAT_ING:
				 printf("recv:%s\n", msg->CHAT.chatbuf);
				 memset(msg, 0, sizeof(msg));//��ջ����ֹ���
				 break;
			 case CHAT_END:
				 printf("the client had exit this system!\n");
				 closeSocket();
				 flag = 1;//��־λ1��˵���˳����߳̽���
				 break;
			 } 
		 }
	 }
 }

