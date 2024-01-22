// Copyright 2021-2022, DearBing. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Json/Public/Dom/JsonObject.h"
#include "Json/Public/Dom/JsonValue.h"
#include "DBJsonBPLibrary.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct FDBStructBase
{
	GENERATED_USTRUCT_BODY()

    FDBStructBase()
	{
	}

	virtual ~FDBStructBase()
	{
	}
};

UCLASS()
class DBJSON_API UDBJsonBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable,  CustomThunk, Category = "DBJson", meta = (CustomStructureParam = "OutStruct",BlueprintInternalUseOnly="true"))
    static bool JsonDeserialize( const UScriptStruct* StructType, const FString& JsonStr, FDBStructBase& OutStruct, FString& ErroInfo);
	static bool Generic_JsonDeserialize( const UScriptStruct* StructType, const FString& JsonStr, void* OutStructPtr, FString& ErroInfo);
	DECLARE_FUNCTION( execJsonDeserialize)
	{
		P_GET_OBJECT( UScriptStruct, StructType );
		P_GET_PROPERTY(FStrProperty, JsonStr);
		Stack.StepCompiledIn<FStructProperty>( NULL );
		void* OutStructPtr = Stack.MostRecentPropertyAddress;
		Stack.StepCompiledIn<FStrProperty>(NULL);
		FString& ErroInfo = *(FString*)Stack.MostRecentPropertyAddress;
		P_FINISH;
		bool bSuccess = false;
		P_NATIVE_BEGIN;
		bSuccess = Generic_JsonDeserialize( StructType, JsonStr, OutStructPtr, ErroInfo);
		P_NATIVE_END;
		*(bool*)RESULT_PARAM = bSuccess;
	}

	UFUNCTION(BlueprintCallable,  CustomThunk, Category = "DBJson", meta = (CustomStructureParam = "Struct",BlueprintInternalUseOnly="true"))
    static FString JsonSerialize( const UScriptStruct* StructType, const FDBStructBase& Struct );
	static bool Generic_JsonSerialize( const UScriptStruct* StructType, void* StructPtr, FString& OutJsonStr );
	DECLARE_FUNCTION( execJsonSerialize)
	{
		P_GET_OBJECT( UScriptStruct, StructType );

		Stack.StepCompiledIn<FStructProperty>( NULL );
		void* StructPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		bool bSuccess = false;
		FString OutJsonStr = "";
		
		P_NATIVE_BEGIN;
		bSuccess = Generic_JsonSerialize( StructType, StructPtr, OutJsonStr );
		P_NATIVE_END;

		*(FString*)RESULT_PARAM = bSuccess ? OutJsonStr : "";
	}
	
#pragma region JsonStringToStruct
	static bool JsonObjectToStruct(TSharedPtr<FJsonObject> Json,const UStruct* StructType, void* StructPtr );

	static bool JsonObjectToUStruct(const TSharedRef<FJsonObject>& JsonObject, const UStruct* StructDefinition, void* OutStruct, int64 CheckFlags = 0, int64 SkipFlags = 0);

	static bool JsonAttributesToUStruct(const TMap< FString, TSharedPtr<FJsonValue> >& JsonAttributes, const UStruct* StructDefinition, void* OutStruct, int64 CheckFlags = 0, int64 SkipFlags = 0);
#pragma endregion

#pragma region StructToJsonString
	static TSharedPtr<FJsonObject> StructToJsonObject( const UStruct* StructType, const void* StructPtr );
	
	static bool UStructToJsonObject(const UStruct* StructDefinition, const void* Struct, TSharedRef<FJsonObject> OutJsonObject, int64 CheckFlags = 0, int64 SkipFlags = 0);

	static FString JsonObjectToString(TSharedPtr<FJsonObject> JsonObject);
#pragma endregion 
	
#pragma region Json
	
private:
	static FString GetShortName(FProperty* Property);

	// json to struct
	static bool JsonAttributesToUStructWithContainer(const TMap< FString, TSharedPtr<FJsonValue> >& JsonAttributes, const UStruct* StructDefinition, void* OutStruct, const UStruct* ContainerStruct, void* Container, int64 CheckFlags = 0, int64 SkipFlags = 0);

	static bool JsonValueToFPropertyWithContainer(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* OutValue, const UStruct* ContainerStruct, void* Container, int64 CheckFlags, int64 SkipFlags);

	static bool ConvertScalarJsonValueToFPropertyWithContainer(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* OutValue, const UStruct* ContainerStruct, void* Container, int64 CheckFlags, int64 SkipFlags);

	// struct to json
	static bool UStructToJsonAttributes(const UStruct* StructDefinition, const void* Struct, TMap< FString, TSharedPtr<FJsonValue> >& OutJsonAttributes, int64 CheckFlags, int64 SkipFlags);

	static TSharedPtr<FJsonValue> FPropertyToJsonValue(FProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags);

	static TSharedPtr<FJsonValue> ConvertScalarFPropertyToJsonValue(FProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags);
#pragma endregion 
};
