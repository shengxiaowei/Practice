// Copyright 2017-2018 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "SocketClient.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Networking/Public/Networking.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include <thread>
#include "SocketClientBPLibrary.generated.h"

class USocketClientHandler;

UCLASS()
class SOCKETCLIENT_API USocketClientBPLibrary : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	~USocketClientBPLibrary();

	//Delegates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FsocketClientConnectionEventDelegate, bool, success, FString, message);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FreceiveTCPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FreceiveUDPMessageEventDelegate, FString, message, const TArray<uint8>&, byteArray);

	UFUNCTION()
		void socketClientConnectionEventDelegate(const bool success, const FString message);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events")
		FsocketClientConnectionEventDelegate onsocketClientConnectionEventDelegate;
	UFUNCTION()
		void receiveTCPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|TCP|Events")
		FreceiveTCPMessageEventDelegate onreceiveTCPMessageEventDelegate;
	UFUNCTION()
		void receiveUDPMessageEventDelegate(const FString message, const TArray<uint8>& byteArray);
	UPROPERTY(BlueprintAssignable, Category = "SocketClient|UDP|Events")
		FreceiveUDPMessageEventDelegate onreceiveUDPMessageEventDelegate;
	
	UFUNCTION(BlueprintCallable, Category = "SocketClient")
		static FString getLocalIP();



	//TCP
	/**
	*Single TCP Connection only! Send a string. TCP connection must exist.
	*@param message String to send
	*@param addLineBreak add a line break at the end
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP", meta = (AutoCreateRefTerm = "byteArray"))
		static void socketClientSendTCPMessage(FString message, TArray<uint8> byteArray, bool addLineBreak = true);
	/**
	*Multi TCP Connection only! Send a string to a specific connection. TCP connection must exist.
	*@param DomainOrIP target IP or Domain
	*@param port target port
	*@param message String to send
	*@param addLineBreak add a line break at the end
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP", meta = (AutoCreateRefTerm = "byteArray"))
		static void socketClientSendTCPMessageTo(FString DomainOrIP, int32 port, FString message, TArray<uint8> byteArray, bool addLineBreak = true);
	/**
	*TCP and UDP connections are closed.
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient")
		void closeSocketClientConnection();
	UFUNCTION(BlueprintCallable, Category = "SocketClient|TCP")
		void connectSocketClient(FString domainOrIP, int32 port);

	bool shouldRun();
	void setRun(bool runP);
	void setTCPSocket(FSocket* socket);
	FSocket* getTCPSocket();
	FSocket* socketTCP;

	//UDP
	/**
	*UDP is Host to Host. Sends a Message to specific target and opens a connection on random port to listen.
	*@param DomainOrIP target IP or Domain
	*@param port target port
	*@param message String to send
	*@param addLineBreak add a line break at the end
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP", meta = (AutoCreateRefTerm = "byteArray"))
		void socketClientSendUDPMessage(FString domainOrIP, int32 port, FString message, TArray<uint8> byteArray, bool addLineBreak = true);
	/**
	*UDP is Host to Host. Opens a connection on specific ip and port and listen on it.
	*@param DomainOrIP IP or Domain to listen
	*@param port port to listen
	*/
	UFUNCTION(BlueprintCallable, Category = "SocketClient|UDP")
		void socketClientInitUDPReceiver(FString domainOrIP, int32 port);
	void UDPReceiver(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);


	void setUDPSocket(FSocket* socket, FUdpSocketReceiver* udpSocketReceiver);
	FSocket* getUDPSocket();
	FSocket* socketUDP;
	FUdpSocketReceiver* getUDPSocketReceiver();
	FUdpSocketReceiver* udpSocketReceiver;

	FString getServerIP();
	int32	getServerPort();

	void setServerIP(FString ip);
	void setServerPort(int32 port);

	/*Sorry Spaghetti code because backward compatibility*/
	static USocketClientBPLibrary* getSocketClientTargetFromSocketCllientHandler(FString IP, int32 Port);
	static void removeSocketClientTargetFromSocketCllientHandler(FString IP, int32 Port);
	static void addSocketClientTargetFromSocketCllientHandler(FString IP, int32 Port, USocketClientBPLibrary* target);


private:

	FString serverIP = FString(TEXT("0.0.0.0"));
	FString serverDomain = FString(TEXT("0.0.0.0"));
	int32	serverPort = 0;
	bool    run;
};

/*********************************************** TCP ***********************************************/
/* asynchronous Thread*/
class FServerConnectionThread : public FRunnable {

public:

	FServerConnectionThread(USocketClientBPLibrary* socketClientP, USocketClientBPLibrary* oldClientP, FString originalIPP) :
		socketClient(socketClientP),
		oldClient(oldClientP),
		originalIP(originalIPP) {
		FString threadName = "FServerConnectionThread" + FGuid::NewGuid().ToString();
		FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {
		//UE_LOG(LogTemp, Display, TEXT("DoWork:%s"),*(FDateTime::Now()).ToString());
		FString ip = socketClient->getServerIP();
		int32 port = socketClient->getServerPort();
		//UE_LOG(LogTemp, Warning, TEXT("Tread:%s:%i"),*ip, port);
		ISocketSubsystem* sSS = ISocketSubsystem::Get();
		TSharedRef<FInternetAddr> addr = sSS->CreateInternetAddr();
		bool bIsValid;
		addr->SetIp(*ip, bIsValid);
		addr->SetPort(port);

		//is already connected? 
		//USocketClientBPLibrary* oldClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
		if (oldClient != nullptr) {
			oldClient->closeSocketClientConnection();
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			//wait for clean disconnect
			int32 maxWaitTime = 200;//max x seconds
			while (maxWaitTime > 0 && oldClient->shouldRun() && USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port) != nullptr) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				maxWaitTime--;
				//UE_LOG(LogTemp, Warning, TEXT("Wait for clean disconnect:%i"), maxWaitTime);
			}
			USocketClientBPLibrary::removeSocketClientTargetFromSocketCllientHandler(ip, port);
			USocketClientBPLibrary::addSocketClientTargetFromSocketCllientHandler(ip, port, socketClient);
		}
		//USocketClientBPLibrary::addSocketClientTargetFromSocketCllientHandler(ip, port, socketClient);
		//USocketClientBPLibrary::addSocketClientTargetFromSocketCllientHandler(originalIP, port, socketClient);





		if (bIsValid) {
			// create the socket
			FSocket* socket = sSS->CreateSocket(NAME_Stream, TEXT("socketClient"));
			socketClient->setTCPSocket(socket);

			// try to connect to the server
			if (socket == nullptr || socket->Connect(*addr) == false) {
				// on failure, shut it all down
				sSS->DestroySocket(socket);
				socket = NULL;
				AsyncTask(ENamedThreads::GameThread, [ip, port]() {
					USocketClientBPLibrary*	socketClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
					if (socketClient != nullptr)
						socketClient->onsocketClientConnectionEventDelegate.Broadcast(false, "Connection failed:" + ip + ":" + FString::FromInt(port));

				});
				return 0;
				//UE_LOG(LogNetworkPlatformFile, Error, TEXT("Failed to connect to file server at %s."), *Addr->ToString(true));
			}
			else {
				AsyncTask(ENamedThreads::GameThread, [ip, port]() {
					USocketClientBPLibrary*	socketClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
					if (socketClient != nullptr)
						socketClient->onsocketClientConnectionEventDelegate.Broadcast(true, "Connection successful:" + ip + ":" + FString::FromInt(port));
				});
				socketClient->setRun(true);

				int64 ticks1;
				int64 ticks2;

				uint32 DataSize;
				while (socket != nullptr && socketClient->shouldRun()) {

					//ESocketConnectionState::SCS_Connected does not work https://issues.unrealengine.com/issue/UE-27542
					//Compare ticks is a workaround to get a disconnect. clientSocket->Wait() stop working after disconnect. (Another bug?)
					//If it doesn't wait any longer, ticks1 and ticks2 should be the same == disconnect.
					ticks1 = FDateTime::Now().GetTicks();
					socket->Wait(ESocketWaitConditions::WaitForReadOrWrite, FTimespan::FromSeconds(0.1));
					ticks2 = FDateTime::Now().GetTicks();
					if (ticks1 == ticks2) {
						UE_LOG(LogTemp, Display, TEXT("TCP connection broken. End Loop"));
						break;
					}


					bool hasData = socket->HasPendingData(DataSize);
					if (hasData) {
						FArrayReaderPtr Datagram = MakeShareable(new FArrayReader(true));
						Datagram->SetNumUninitialized(DataSize);
						int32 BytesRead = 0;
						if (socket->Recv(Datagram->GetData(), Datagram->Num(), BytesRead)) {

							char* Data = (char*)Datagram->GetData();
							Data[BytesRead] = '\0';
							FString recvMessage = FString(UTF8_TO_TCHAR(Data));

							TArray<uint8> byteArray;
							for (int i = 0; i < Datagram->Num(); i++) {
								byteArray.Add(Datagram->GetData()[i]);
							}

							//switch to gamethread
							AsyncTask(ENamedThreads::GameThread, [recvMessage, byteArray, ip, port]() {
								USocketClientBPLibrary*	socketClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
								if (socketClient != nullptr)
									socketClient->onreceiveTCPMessageEventDelegate.Broadcast(recvMessage, byteArray);
							});
						}
						Datagram->Empty();
					}
				}

				//UE_LOG(LogTemp, Warning, TEXT("Close Socket 1"));
				FString originalIPLambda = originalIP;
				AsyncTask(ENamedThreads::GameThread, [originalIPLambda, ip, port]() {
					//UE_LOG(LogTemp, Warning, TEXT("Close Socket 2"));
					USocketClientBPLibrary*	socketClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
					if (socketClient != nullptr) {
						socketClient->onsocketClientConnectionEventDelegate.Broadcast(false, "Connection close:" + ip + ":" + FString::FromInt(port));
						socketClient->setRun(false);
					}
					//UE_LOG(LogTemp, Warning, TEXT("Close Socket 3"));
					USocketClientBPLibrary::removeSocketClientTargetFromSocketCllientHandler(ip, port);
					//USocketClientBPLibrary::removeSocketClientTargetFromSocketCllientHandler(originalIPLambda, port);

				});
				//UE_LOG(LogTemp, Warning, TEXT("Close Socket 4"));
				socketClient->setRun(false);
				sSS->DestroySocket(socket);
				socket = NULL;
				UE_LOG(LogTemp, Display, TEXT("Close Socket"));
			}
		}

		return 0;
	}


protected:
	USocketClientBPLibrary * socketClient;
	USocketClientBPLibrary*		oldClient;
	FString originalIP;

};




/* asynchronous Thread*/
class FSendDataToServerThread : public FRunnable {

public:

	FSendDataToServerThread(FString messageP, TArray<uint8> byteArrayP, USocketClientBPLibrary* socketClientP, FString ipP, int32 portP) :
		message(messageP),
		byteArray(byteArrayP),
		socketClient(socketClientP),
		ipGlobal(ipP),
		portGlobal(portP) {
		FString threadName = "FSendDataToServerThread" + FGuid::NewGuid().ToString();
		FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {
		FString ip = ipGlobal;
		int32 port = portGlobal;

		if (socketClient == nullptr) {
			UE_LOG(LogTemp, Error, TEXT("Class ist not initialized."));
			return 0;
		}

		// get the socket
		FSocket* socket = socketClient->getTCPSocket();

		// try to connect to the server

		if (socket == NULL || socket == nullptr || socketClient->shouldRun() == false) {
			UE_LOG(LogTemp, Error, TEXT("Connection not exist."));
			AsyncTask(ENamedThreads::GameThread, [ip, port]() {
				USocketClientBPLibrary*	socketClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
				if (socketClient != nullptr) {
					socketClient->onsocketClientConnectionEventDelegate.Broadcast(false, "Connection not exist:" + ip + ":" + FString::FromInt(port));
					socketClient->closeSocketClientConnection();
				}
			});
			return 0;
		}

		
		if (socket != nullptr && socket->GetConnectionState() == ESocketConnectionState::SCS_Connected) {
			if (message.Len() > 0) {
				TCHAR *serializedChar = message.GetCharArray().GetData();
				int32 size = FCString::Strlen(serializedChar);
				int32 sent = 0;
				socket->Send((uint8*)TCHAR_TO_UTF8(serializedChar), size, sent);
			}
			if (byteArray.Num() > 0) {
				int32 sent = 0;
				socket->Send(byteArray.GetData(), byteArray.Num(),sent);
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Connection Lost"));
			AsyncTask(ENamedThreads::GameThread, [ip, port]() {
				USocketClientBPLibrary*	socketClient = USocketClientBPLibrary::getSocketClientTargetFromSocketCllientHandler(ip, port);
				if (socketClient != nullptr)
					socketClient->onsocketClientConnectionEventDelegate.Broadcast(false, "Connection lost:" + ip + ":" + FString::FromInt(port));
			});
		}

		return 0;
	}


protected:
	FString message;
	TArray<uint8> byteArray;
	USocketClientBPLibrary* socketClient;
	FString ipGlobal;
	int32 portGlobal;

};


/*********************************************** UDP ***********************************************/
/* asynchronous Thread*/
class FServerUDPConnectionThread : public FRunnable {

public:

	FServerUDPConnectionThread(FString messageP, TArray<uint8> byteArrayP, USocketClientBPLibrary* socketClientP) :
		message(messageP),
		byteArray(byteArrayP),
		socketClient(socketClientP) {
		FString threadName = "FServerUDPConnectionThread_" + FGuid::NewGuid().ToString();
		FRunnableThread::Create(this, *threadName, 0, EThreadPriority::TPri_Normal);
	}

	virtual uint32 Run() override {
		if (socketClient == nullptr) {
			UE_LOG(LogTemp, Error, TEXT("Class ist not initialized."));
			return 0;
		}

		FSocket* socket = socketClient->getUDPSocket();
		FUdpSocketReceiver* udpSocketReceiver = socketClient->getUDPSocketReceiver();

		//init udp receiver and socket exisist? then dissconnect
		if (message.Len() <= 0 && byteArray.Num() <=0 && socket != nullptr && socket != NULL) {
			socket->Close();
			socket = NULL;

			if (udpSocketReceiver != nullptr && udpSocketReceiver != NULL) {
				udpSocketReceiver->Stop();
				udpSocketReceiver->Exit();
				udpSocketReceiver = NULL;
			}

			socketClient->setUDPSocket(NULL, NULL);
		}




		if (socket == nullptr || socket == NULL) {
			FString ip = socketClient->getServerIP();
			int32 port = socketClient->getServerPort();

			FString endpointAdress = ip + ":" + FString::FromInt(port);
			FIPv4Endpoint Endpoint;

			// create the socket
			FString socketName;
			//if you send a message without init udp receiver first than a random port is set
			if (message.Len() <= 0 && byteArray.Num() <=0 && FIPv4Endpoint::Parse(endpointAdress, Endpoint)) {
				socket = FUdpSocketBuilder(*socketName).AsReusable().WithBroadcast().BoundToEndpoint(Endpoint);
			}
			else {
				socket = FUdpSocketBuilder(*socketName).AsReusable().WithBroadcast();
			}


			FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
			udpSocketReceiver = new FUdpSocketReceiver(socket, ThreadWaitTime, TEXT("SocketClientBPLibUDPReceiverThread"));
			udpSocketReceiver->OnDataReceived().BindUObject(socketClient, &USocketClientBPLibrary::UDPReceiver);
			udpSocketReceiver->Start();

			socketClient->setUDPSocket(socket, udpSocketReceiver);
		}



		if (message.Len() > 0 || byteArray.Num() > 0) {

			TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get()->CreateInternetAddr();
			bool bIsValid;
			addr->SetIp(*socketClient->getServerIP(), bIsValid);
			addr->SetPort(socketClient->getServerPort());
			if (bIsValid) {
				if (message.Len() > 0) {
					TCHAR *serializedChar = message.GetCharArray().GetData();
					int32 size = FCString::Strlen(serializedChar);
					int32 sent = 0;
					socket->SendTo((uint8*)TCHAR_TO_UTF8(serializedChar), size, sent, *addr);
				}
				if (byteArray.Num() > 0) {
					int32 sent = 0;
					socket->SendTo(byteArray.GetData(), byteArray.Num(), sent, *addr);
				}
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("UDP Connection fail."));
			}

		}

		return 0;
	}


protected:
	FString message;
	TArray<uint8> byteArray;
	USocketClientBPLibrary*		socketClient;

};