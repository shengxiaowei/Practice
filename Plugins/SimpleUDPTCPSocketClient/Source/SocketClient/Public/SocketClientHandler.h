// Copyright 2017-2018 David Romanski (Socke). All Rights Reserved.
#pragma once

#include "SocketClient.h"
#include "SocketClientHandler.generated.h"


class USocketClientBPLibrary;


USTRUCT(BlueprintType)
struct FClientSocketTCPStruct
{
	GENERATED_USTRUCT_BODY()

	FString ip;
	int32	port;
	//FString sessionID;
	USocketClientBPLibrary* socketClientBPLibrary;
};

UCLASS(Blueprintable, BlueprintType)
class USocketClientHandler : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	//UPROPERTY()
	//	FString InaccessibleUProperty;

	static USocketClientHandler *socketClientHandler;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient")
		static USocketClientBPLibrary* getSocketClientTarget();

	/**
	*Use this only for the multi TCP connections.
	*@param DomainOrIP IP or domain of an existing TCP connection
	*@param port Port of an existing TCP connection
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SocketClient")
		static void getSocketClientTargetByIP_AndPort(const FString DomainOrIP, const int32 Port, bool &found, USocketClientBPLibrary* &target);

	static USocketClientBPLibrary* getSocketClientTargetByIP_AndPortInternal(FString IP, int32 Port);
	static void removeSocketClientTargetByIP_AndPortInternal(FString IP, int32 Port);

	static FString resolveDomain(FString domain);


	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FJavascriptEventTag, const FString, ID, const FString, ClassName, const FString, Value, const TArray<FString>&, args);
	//UFUNCTION()
	//	void JavascriptEventTag(const FString ID, const FString ClassName, const FString Value, const TArray<FString>& args);
	//UPROPERTY(BlueprintAssignable, Category = "HTML Menu|Events")
	//	FJavascriptEventTag onJavascriptEventTag;

	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJavascriptEventBlank, const TArray<FString>&, args);
	//UFUNCTION()
	//	void JavascriptEventBlank(const TArray<FString>& args);
	//UPROPERTY(BlueprintAssignable, Category = "HTML Menu|Events")
	//	FJavascriptEventBlank onJavascriptEventBlank;

	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJavascriptEventMap, const FStringTMap, map);
	//UFUNCTION()
	//	void JavascriptEventMap(const FStringTMap map);
	//UPROPERTY(BlueprintAssignable, Category = "HTML Menu|Events")
	//	FJavascriptEventMap onJavascriptEventMap;

	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJavascriptEventJSON, const FString, json);
	//UFUNCTION()
	//	void JavascriptEventJSON(const FString json);
	//UPROPERTY(BlueprintAssignable, Category = "HTML Menu|Events")
	//	FJavascriptEventJSON onJavascriptEventJSON;


	///**
	//* UE4 does not support maps in events. Therefore, this intermediate step.
	//*/
	//UFUNCTION(BlueprintCallable, Category = "HTML Menu")
	//	static TMap<FString, FString> toStringMap(FStringTMap map);


	//void setBrowser(UHTMLMenu* browser);

	static void addTCPClientToMap(FString IP, int32 Port, USocketClientBPLibrary* client);

private:
	//UHTMLMenu* browser;
protected:
	UPROPERTY()
	TMap<FString, FClientSocketTCPStruct> TCP_ClientsMap;
};

