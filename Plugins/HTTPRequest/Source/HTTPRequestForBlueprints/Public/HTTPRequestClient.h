// Copyright (c) 2023 King Wai Mark


#pragma once

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HTTPRequestClient.generated.h"


/**
 *	The types of request methods available.
 *
 *	@author King Wai Mark
 */
UENUM(BlueprintType)
enum class EMethod : uint8
{
	GET,
	POST,
	PUT,
	PATCH,
	DEL UMETA(DisplayName = "DELETE"),
	COPY,
	HEAD,
	OPTIONS,
	LINK,
	UNLINK,
	LOCK,
	UNLOCK,
	PROPFIND,
	VIEW
	
};

/**
 *	@author King Wai Mark
 *
 *	JSON Object that is readable for blueprints.
 * 
 */
USTRUCT(BlueprintType)
struct FJSONObject
{
	GENERATED_BODY()
	
	TSharedPtr<FJsonObject> JsonParsed = nullptr;
	
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FResponse, uint8, Status, FString, ResponseString);

/**
 * @author King Wai Mark
 */
UCLASS()
class HTTPREQUESTFORBLUEPRINTS_API UHTTPRequestClient : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 *	Set the timeout duration in seconds for the request. Should stop the request.
	 *
	 *	Cannot be less than 3 seconds.
	 */
	static int RequestTimeoutDuration;

	/**
	 *	Converts the EMethod emum value to a String.
	 *	@author King Wai Mark
	 */
	static FString MethodToString(EMethod Method);

	/**
	 *	The start of a HTTP Request.
	 *
	 *	@param Method The request Verb, GET, PUSH, POST, DELETE, etc.
	 *	@param URL 
	 *	@param Params Query params, ?param1=...&param2=...
	 *	@param Headers Request Headers
	 *	@param Body Content for the request
	 *	@param OnComplete Callback
	 *
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Make a HTTP Request", Keywords = "Make a HTTP Request"), Category = "HTTP Requests For Blueprint Plugin")
	static void MakeAHttpRequest(const EMethod Method, const FString URL, const TMap<FString, FString> Params, const TMap<FString, FString> Headers, const FString Body, const FResponse &OnComplete);

	/**
	 *	Converts the Response String from the Make a HTTP Request callback into a JSON Object that can be used in a blueprint.
	 *
	 *	@param ResponseString The response string returned from Make a HTTP Request
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "To JSON", Keywords = "Response String to JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "JSON") FJSONObject ResponseStringToJSON(const FString ResponseString);

	/**
	 *	Attempts to get a Object value from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Object", Keywords = "Get Object from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "JSON") FJSONObject GetObjectFromJSONObject(const FJSONObject JSON, const FString Key);
	/**
	 *	Attempts to get a Array of JSON objects from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Object Array", Keywords = "Get Object Array from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "Array of JSON") TArray<FJSONObject> GetObjectArrayFromJSONObject(const FJSONObject JSON, const FString Key);
	
	/**
	 *	Attempts to get the String Array from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get String Array", Keywords = "Get String Array from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "Array of Strings") TArray<FString> GetStringArrayFromJSONObject(const FJSONObject JSON, const FString Key);
	/**
	 *	Attempts to get the Number Array from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Number Array", Keywords = "Get Number Array from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "Array of Numbers") TArray<float> GetNumberArrayFromJSONObject(const FJSONObject JSON, const FString Key);

	/**
	 *	Attempts to get the Boolean Array from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Boolean Array", Keywords = "Get Boolean Array from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "Array of Boolean") TArray<bool> GetBooleanArrayFromJSONObject(const FJSONObject JSON, const FString Key);

	/**
	 *	Attempts to get a string value from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get String", Keywords = "Get String from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "String") FString GetStringFromJSONObject(const FJSONObject JSON, const FString Key);

	/**
	 *	Attempts to get a number value from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Number", Keywords = "Get Number from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "Number") float GetNumberFromJSONObject(const FJSONObject JSON, const FString Key);

	/**
	 *	Attempts to get a boolean value from the JSON object.
	 *
	 *	@param JSON JSON Object from the HTTP Requests For Blueprint Plugin
	 *	@param Key Name of the string from JSON
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Boolean", Keywords = "Get Boolean from JSON"), Category = "HTTP Requests For Blueprint Plugin")
	static UPARAM(DisplayName = "Boolean") bool GetBooleanFromJSONObject(const FJSONObject JSON, const FString Key);


	/**
	 *	Sets the timeout duration of the request.
	 *	
	 *	Default is 60 seconds.
	 *
	 *	@param TimeoutDuration Duration in seconds. Cannot be less than 3.
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Timeout Duration", Keywords = "Set Timeout Duration"), Category = "HTTP Requests For Blueprint Plugin")
	static void SetTimeoutDuration(const int TimeoutDuration);

	/**
	 *	Gets the current timeout duration.
	 *	
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Timeout Duration", Keywords = "Get Timeout Duration"), Category = "HTTP Requests For Blueprint Plugin")
	static int GetTimeoutDuration();
	
	/**
	 *	Converts a string to base 64 url.
	 *	
	 *	
	 *	@author King Wai Mark
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Replace With ASCII", Keywords = "Replace With ASCII"), Category = "HTTP Requests For Blueprint Plugin")
	static FString ReplaceWithASCII(const FString& Str);


};
