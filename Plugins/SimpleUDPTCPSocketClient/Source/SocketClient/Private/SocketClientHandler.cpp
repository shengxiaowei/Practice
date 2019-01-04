// Copyright 2017-2018 David Romanski (Socke). All Rights Reserved.

#include "SocketClientHandler.h"

USocketClientHandler* USocketClientHandler::socketClientHandler;

USocketClientHandler::USocketClientHandler(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	socketClientHandler = this;
}


USocketClientBPLibrary * USocketClientHandler::getSocketClientTarget(){
	//if it is empty than create defaut instance because backward compatibility 

	if (USocketClientHandler::socketClientHandler->TCP_ClientsMap.Num() == 0) {
		UPROPERTY()
		USocketClientBPLibrary* socketClientBPLibrary = NewObject<USocketClientBPLibrary>(USocketClientBPLibrary::StaticClass());
		socketClientBPLibrary->AddToRoot();
		USocketClientHandler::socketClientHandler->addTCPClientToMap("0.0.0.0", 0, socketClientBPLibrary);
		return socketClientBPLibrary;
	}

	//allways last one in the map
	TArray<FString> keys;
	USocketClientHandler::socketClientHandler->TCP_ClientsMap.GetKeys(keys);
	FClientSocketTCPStruct* sPointer = USocketClientHandler::socketClientHandler->TCP_ClientsMap.Find(keys[keys.Num()-1]);
	if (sPointer != nullptr) {
		FClientSocketTCPStruct s = *sPointer;
		if (s.socketClientBPLibrary->IsValidLowLevel())
			return s.socketClientBPLibrary;
	}
	return nullptr;
}

void USocketClientHandler::getSocketClientTargetByIP_AndPort(const FString IP, const int32 Port, bool &found, USocketClientBPLibrary* &target) {
	found = false;
	if (IP.IsEmpty() || USocketClientHandler::socketClientHandler->TCP_ClientsMap.Num() == 0) {
		//UE_LOG(LogTemp, Warning, TEXT("getSocketClientTargetByIP_AndPort 1"));
		return;
	}

	FString key = resolveDomain(IP) + ":" + FString::FromInt(Port);
	FClientSocketTCPStruct* sPointer = USocketClientHandler::socketClientHandler->TCP_ClientsMap.Find(key);
	if (sPointer != nullptr) {
		
		FClientSocketTCPStruct s = *sPointer;
		if (s.socketClientBPLibrary->IsValidLowLevel()) {
			target = s.socketClientBPLibrary;
			found = true;
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("getSocketClientTargetByIP_AndPort 2"));
}

USocketClientBPLibrary* USocketClientHandler::getSocketClientTargetByIP_AndPortInternal(FString IP, int32 Port){
	if (IP.IsEmpty()) {
		//UE_LOG(LogTemp, Warning, TEXT("getSocketClientTargetByIP_AndPortInternal 1"));
		return nullptr;
	}
	FString key = IP + ":" + FString::FromInt(Port);
	FClientSocketTCPStruct* sPointer = USocketClientHandler::socketClientHandler->TCP_ClientsMap.Find(key);
	if (sPointer != nullptr) {
		//UE_LOG(LogTemp, Warning, TEXT("getSocketClientTargetByIP_AndPortInternal 2"));
		FClientSocketTCPStruct s = *sPointer;
		if (s.socketClientBPLibrary->IsValidLowLevel())
			return s.socketClientBPLibrary;
	}
	//UE_LOG(LogTemp, Warning, TEXT("getSocketClientTargetByIP_AndPortInternal 3"));
	return nullptr;
}


void USocketClientHandler::removeSocketClientTargetByIP_AndPortInternal(FString IP, int32 Port) {
	if (IP.IsEmpty()) {
		return;
	}

	FString key = IP + ":" + FString::FromInt(Port);
	FClientSocketTCPStruct* sPointer = USocketClientHandler::socketClientHandler->TCP_ClientsMap.Find(key);
	if (sPointer != nullptr) {
		FClientSocketTCPStruct s = *sPointer;
		//s.socketClientBPLibrary->RemoveFromRoot();
		USocketClientHandler::socketClientHandler->TCP_ClientsMap.Remove(key);
	}
	return;
}

FString USocketClientHandler::resolveDomain(FString serverDomain){

	//is IP
	TArray<FString> ipNumbers;
	int32 lineCount = serverDomain.ParseIntoArray(ipNumbers, TEXT("."), true);
	if (lineCount == 4 && serverDomain.Len() <= 15 && serverDomain.Len() >= 7) {
		return serverDomain;
	}

	//resolve Domain
	ISocketSubsystem* sSS = ISocketSubsystem::Get();

	auto ResolveInfo = sSS->GetHostByName(TCHAR_TO_ANSI(*serverDomain));
	while (!ResolveInfo->IsComplete());

	if (ResolveInfo->GetErrorCode() == 0) {
		const FInternetAddr* Addr = &ResolveInfo->GetResolvedAddress();
		uint32 OutIP = 0;
		return Addr->ToString(false);
	}

	return serverDomain;
}

void USocketClientHandler::addTCPClientToMap(FString IP, int32 Port, USocketClientBPLibrary* client) {
	if (IP.IsEmpty()) {
		return;
	}
	FString key = IP + ":" + FString::FromInt(Port);
	FClientSocketTCPStruct s;
	s.ip = IP;
	s.port = Port;
	s.socketClientBPLibrary = client;

	USocketClientHandler::socketClientHandler->TCP_ClientsMap.Add(key, s);
}