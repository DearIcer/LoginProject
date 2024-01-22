// Copyright 2021-2022, DearBing. All Rights Reserved.

#include "DBJsonBPLibrary.h"
#include "DBJson.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/Package.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Runtime/Launch/Resources/Version.h"


bool UDBJsonBPLibrary::JsonDeserialize(const UScriptStruct* StructType, const FString& JsonStr,FDBStructBase& OutStruct, FString& ErroInfo)
{
	check(0);
	return false;
}

bool UDBJsonBPLibrary::Generic_JsonDeserialize(const UScriptStruct* StructType, const FString& JsonStr,void* OutStructPtr, FString& ErroInfo)
{
	if ( !StructType || !OutStructPtr )
		return false;
	TSharedPtr<FJsonObject> Object = MakeShareable(new FJsonObject);
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Object))
	{
		ErroInfo = Reader->GetErrorMessage();
		return false;
	}
	if(Object == nullptr)
		return false;	
	
	return JsonObjectToStruct(Object,StructType,OutStructPtr);
}

FString UDBJsonBPLibrary::JsonSerialize(const UScriptStruct* StructType, const FDBStructBase& Struct)
{
	check( 0 );
	return "";
}

bool UDBJsonBPLibrary::Generic_JsonSerialize(const UScriptStruct* StructType, void* StructPtr, FString& OutJsonStr)
{
	if ( !StructType || !StructPtr )
		return false;

	TSharedPtr<FJsonObject> Object = StructToJsonObject(StructType,StructPtr);;
	if(!Object)
		return false;
	
	OutJsonStr = JsonObjectToString(Object);
	return !OutJsonStr.IsEmpty();
}

bool UDBJsonBPLibrary::JsonObjectToStruct(TSharedPtr<FJsonObject> Json, const UStruct* StructType, void* StructPtr)
{
	return JsonObjectToUStruct(Json.ToSharedRef(), StructType, StructPtr );
}

bool UDBJsonBPLibrary::JsonObjectToUStruct(const TSharedRef<FJsonObject>& JsonObject, const UStruct* StructDefinition,
	void* OutStruct, int64 CheckFlags, int64 SkipFlags)
{
	return JsonAttributesToUStruct(JsonObject->Values, StructDefinition, OutStruct, CheckFlags, SkipFlags);
}

bool UDBJsonBPLibrary::JsonAttributesToUStruct(const TMap<FString, TSharedPtr<FJsonValue>>& JsonAttributes,
	const UStruct* StructDefinition, void* OutStruct, int64 CheckFlags, int64 SkipFlags)
{
	return JsonAttributesToUStructWithContainer(JsonAttributes, StructDefinition, OutStruct, StructDefinition, OutStruct, CheckFlags, SkipFlags);
}

TSharedPtr<FJsonObject> UDBJsonBPLibrary::StructToJsonObject(const UStruct* StructType, const void* StructPtr)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();;
	if ( StructType && StructPtr )
	{
		UStructToJsonObject(StructType,StructPtr,Json.ToSharedRef());

	}
	return Json;
}

bool UDBJsonBPLibrary::UStructToJsonObject(const UStruct* StructDefinition, const void* Struct, TSharedRef<FJsonObject> OutJsonObject, int64 CheckFlags, int64 SkipFlags)
{
	return UStructToJsonAttributes(StructDefinition, Struct, OutJsonObject->Values, CheckFlags, SkipFlags);
}

FString UDBJsonBPLibrary::JsonObjectToString(TSharedPtr<FJsonObject> JsonObject)
{
	FString Result = "";
	if (JsonObject.IsValid())
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result, 0);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
		JsonWriter->Close();
	}
	return Result;	
}

bool UDBJsonBPLibrary::JsonAttributesToUStructWithContainer(const TMap< FString, TSharedPtr<FJsonValue> >& JsonAttributes, const UStruct* StructDefinition, void* OutStruct, const UStruct* ContainerStruct, void* Container, int64 CheckFlags, int64 SkipFlags)
{
	if (StructDefinition == FJsonObjectWrapper::StaticStruct())
	{
		// Just copy it into the object
		FJsonObjectWrapper* ProxyObject = (FJsonObjectWrapper*)OutStruct;
		ProxyObject->JsonObject = MakeShared<FJsonObject>();
		ProxyObject->JsonObject->Values = JsonAttributes;
		return true;
	}

	int32 NumUnclaimedProperties = JsonAttributes.Num();
	if (NumUnclaimedProperties <= 0)
	{
		return true;
	}

	// iterate over the struct properties
	for (TFieldIterator<FProperty> PropIt(StructDefinition); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

		// Check to see if we should ignore this property
		if (CheckFlags != 0 && !Property->HasAnyPropertyFlags(CheckFlags))
		{
			continue;
		}
		if (Property->HasAnyPropertyFlags(SkipFlags))
		{
			continue;
		}

		// find a json value matching this property name
		const TSharedPtr<FJsonValue>* JsonValue = JsonAttributes.Find(GetShortName(Property));
		if (!JsonValue)
		{
			// we allow values to not be found since this mirrors the typical UObject mantra that all the fields are optional when deserializing
			continue;
		}

		if (JsonValue->IsValid() && !(*JsonValue)->IsNull())
		{
			void* Value = Property->ContainerPtrToValuePtr<uint8>(OutStruct);
			if (!JsonValueToFPropertyWithContainer(*JsonValue, Property, Value, nullptr, nullptr, CheckFlags, SkipFlags))
			{
				UE_LOG(LogJson, Error, TEXT("JsonObjectToUStruct - Unable to parse %s.%s from JSON"), *StructDefinition->GetName(), *Property->GetName());
				return false;
			}
		}

		if (--NumUnclaimedProperties <= 0)
		{
			// If we found all properties that were in the JsonAttributes map, there is no reason to keep looking for more.
			break;
		}
	}

	return true;
}

FString UDBJsonBPLibrary::GetShortName(FProperty* Property)
{
	FString PropertyName = Property->GetName();;
	int LastIndex = 0;
	if (PropertyName.FindLastChar('_', LastIndex))
	{
		PropertyName = PropertyName.Mid(0, LastIndex);
		if (PropertyName.FindLastChar('_', LastIndex))
		{
			return PropertyName.Mid(0, LastIndex);
		}
	}
	return PropertyName;
}


// Convert JSON to property, assuming either the property is not an array or the value is an individual array element #1#
bool UDBJsonBPLibrary::JsonValueToFPropertyWithContainer(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* OutValue, const UStruct* ContainerStruct, void* Container, int64 CheckFlags, int64 SkipFlags)
{
	if (!JsonValue.IsValid())
	{
		UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Invalid value JSON key"));
		return false;
	}

	bool bArrayOrSetProperty = Property->IsA<FArrayProperty>() || Property->IsA<FSetProperty>();
	bool bJsonArray = JsonValue->Type == EJson::Array;

	if (!bJsonArray)
	{
		if (bArrayOrSetProperty)
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import TArray from non-array JSON key"));
			return false;
		}

		if (Property->ArrayDim != 1)
		{
			UE_LOG(LogJson, Warning, TEXT("Ignoring excess properties when deserializing %s"), *Property->GetName());
		}

		return ConvertScalarJsonValueToFPropertyWithContainer(JsonValue, Property, OutValue, ContainerStruct, Container, CheckFlags, SkipFlags);
	}

	// In practice, the ArrayDim == 1 check ought to be redundant, since nested arrays of FPropertys are not supported
	if (bArrayOrSetProperty && Property->ArrayDim == 1)
	{
		// Read into TArray
		return ConvertScalarJsonValueToFPropertyWithContainer(JsonValue, Property, OutValue, ContainerStruct, Container, CheckFlags, SkipFlags);
	}

	// We're deserializing a JSON array
	const auto& ArrayValue = JsonValue->AsArray();
	if (Property->ArrayDim < ArrayValue.Num())
	{
		UE_LOG(LogJson, Warning, TEXT("Ignoring excess properties when deserializing %s"), *Property->GetName());
	}

	// Read into native array
	int ItemsToRead = FMath::Clamp(ArrayValue.Num(), 0, Property->ArrayDim);
	for (int Index = 0; Index != ItemsToRead; ++Index)
	{
		if (!ConvertScalarJsonValueToFPropertyWithContainer(ArrayValue[Index], Property, (char*)OutValue + Index * Property->ElementSize, ContainerStruct, Container, CheckFlags, SkipFlags))
		{
			return false;
		}
	}
	return true;
}


bool UDBJsonBPLibrary::ConvertScalarJsonValueToFPropertyWithContainer(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* OutValue, const UStruct* ContainerStruct, void* Container, int64 CheckFlags, int64 SkipFlags)
{
	if (FEnumProperty * EnumProperty = CastField<FEnumProperty>(Property))
	{
		if (JsonValue->Type == EJson::String)
		{
			// see if we were passed a string for the enum
			const UEnum* Enum = EnumProperty->GetEnum();
			check(Enum);
			FString StrValue = JsonValue->AsString();
			int64 IntValue = Enum->GetValueByName(FName(*StrValue));
			if (IntValue == INDEX_NONE)
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable import enum %s from string value %s for property %s"), *Enum->CppType, *StrValue, *Property->GetNameCPP());
				return false;
			}
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(OutValue, IntValue);
		}
		else
		{
			// AsNumber will log an error for completely inappropriate types (then give us a default)
			EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(OutValue, (int64)JsonValue->AsNumber());
		}
	}
	else if (FNumericProperty * NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (NumericProperty->IsEnum() && JsonValue->Type == EJson::String)
		{
			// see if we were passed a string for the enum
			const UEnum* Enum = NumericProperty->GetIntPropertyEnum();
			check(Enum); // should be assured by IsEnum()
			FString StrValue = JsonValue->AsString();
			int64 IntValue = Enum->GetValueByName(FName(*StrValue));
			if (IntValue == INDEX_NONE)
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable import enum %s from string value %s for property %s"), *Enum->CppType, *StrValue, *Property->GetNameCPP());
				return false;
			}
			NumericProperty->SetIntPropertyValue(OutValue, IntValue);
		}
		else if (NumericProperty->IsFloatingPoint())
		{
			// AsNumber will log an error for completely inappropriate types (then give us a default)
			NumericProperty->SetFloatingPointPropertyValue(OutValue, JsonValue->AsNumber());
		}
		else if (NumericProperty->IsInteger())
		{
			if (JsonValue->Type == EJson::String)
			{
				// parse string -> int64 ourselves so we don't lose any precision going through AsNumber (aka double)
				NumericProperty->SetIntPropertyValue(OutValue, FCString::Atoi64(*JsonValue->AsString()));
			}
			else
			{
				// AsNumber will log an error for completely inappropriate types (then give us a default)
				NumericProperty->SetIntPropertyValue(OutValue, (int64)JsonValue->AsNumber());
			}
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable to set numeric property type %s for property %s"), *Property->GetClass()->GetName(), *Property->GetNameCPP());
			return false;
		}
	}
	else if (FBoolProperty * BoolProperty = CastField<FBoolProperty>(Property))
	{
		BoolProperty->SetPropertyValue(OutValue, JsonValue->AsBool());
	}
	else if (FStrProperty * StringProperty = CastField<FStrProperty>(Property))
	{
		StringProperty->SetPropertyValue(OutValue, JsonValue->AsString());
	}
	else if (FArrayProperty * ArrayProperty = CastField<FArrayProperty>(Property))
	{
		if (JsonValue->Type == EJson::Array)
		{
			TArray< TSharedPtr<FJsonValue> > ArrayValue = JsonValue->AsArray();
			int32 ArrLen = ArrayValue.Num();
			FScriptArrayHelper Helper(ArrayProperty, OutValue);
			Helper.Resize(ArrLen);
			for (int32 i = 0; i < ArrLen; ++i)
			{
				const TSharedPtr<FJsonValue>& ArrayValueItem = ArrayValue[i];
				if (ArrayValueItem.IsValid() && !ArrayValueItem->IsNull())
				{
					if (!JsonValueToFPropertyWithContainer(ArrayValueItem, ArrayProperty->Inner, Helper.GetRawPtr(i), ContainerStruct, Container, CheckFlags & (~CPF_ParmFlags), SkipFlags))
					{
						UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable to deserialize array element [%d] for property %s"), i, *Property->GetNameCPP());
						return false;
					}
				}
			}
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import TArray from non-array JSON key for property %s"), *Property->GetNameCPP());
			return false;
		}
	}
	else if (FMapProperty * MapProperty = CastField<FMapProperty>(Property))
	{
		if (JsonValue->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> ObjectValue = JsonValue->AsObject();

			FScriptMapHelper Helper(MapProperty, OutValue);

			check(ObjectValue);

			int32 MapSize = ObjectValue->Values.Num();
			Helper.EmptyValues(MapSize);

			// set the property values
			for (const auto& Entry : ObjectValue->Values)
			{
				if (Entry.Value.IsValid() && !Entry.Value->IsNull())
				{
					int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();

					TSharedPtr<FJsonValueString> TempKeyValue = MakeShared<FJsonValueString>(Entry.Key);

					const bool bKeySuccess = JsonValueToFPropertyWithContainer(TempKeyValue, MapProperty->KeyProp, Helper.GetKeyPtr(NewIndex), ContainerStruct, Container, CheckFlags & (~CPF_ParmFlags), SkipFlags);
					const bool bValueSuccess = JsonValueToFPropertyWithContainer(Entry.Value, MapProperty->ValueProp, Helper.GetValuePtr(NewIndex), ContainerStruct, Container, CheckFlags & (~CPF_ParmFlags), SkipFlags);

					if (!(bKeySuccess && bValueSuccess))
					{
						UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable to deserialize map element [key: %s] for property %s"), *Entry.Key, *Property->GetNameCPP());
						return false;
					}
				}
			}

			Helper.Rehash();
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import TMap from non-object JSON key for property %s"), *Property->GetNameCPP());
			return false;
		}
	}
	else if (FSetProperty * SetProperty = CastField<FSetProperty>(Property))
	{
		if (JsonValue->Type == EJson::Array)
		{
			TArray< TSharedPtr<FJsonValue> > ArrayValue = JsonValue->AsArray();
			int32 ArrLen = ArrayValue.Num();

			FScriptSetHelper Helper(SetProperty, OutValue);

			// set the property values
			for (int32 i = 0; i < ArrLen; ++i)
			{
				const TSharedPtr<FJsonValue>& ArrayValueItem = ArrayValue[i];
				if (ArrayValueItem.IsValid() && !ArrayValueItem->IsNull())
				{
					int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
					if (!JsonValueToFPropertyWithContainer(ArrayValueItem, SetProperty->ElementProp, Helper.GetElementPtr(NewIndex), ContainerStruct, Container, CheckFlags & (~CPF_ParmFlags), SkipFlags))
					{
						UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable to deserialize set element [%d] for property %s"), i, *Property->GetNameCPP());
						return false;
					}
				}
			}

			Helper.Rehash();
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import TSet from non-array JSON key for property %s"), *Property->GetNameCPP());
			return false;
		}
	}
	else if (FTextProperty * TextProperty = CastField<FTextProperty>(Property))
	{
		if (JsonValue->Type == EJson::String)
		{
			// assume this string is already localized, so import as invariant
			TextProperty->SetPropertyValue(OutValue, FText::FromString(JsonValue->AsString()));
		}
		else if (JsonValue->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> Obj = JsonValue->AsObject();
			check(Obj.IsValid()); // should not fail if Type == EJson::Object

								  // import the subvalue as a culture invariant string
			FText Text;
			if (!FJsonObjectConverter::GetTextFromObject(Obj.ToSharedRef(), Text))
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import FText from JSON object with invalid keys for property %s"), *Property->GetNameCPP());
				return false;
			}
			TextProperty->SetPropertyValue(OutValue, Text);
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import FText from JSON that was neither string nor object for property %s"), *Property->GetNameCPP());
			return false;
		}
	}
	else if (FStructProperty * StructProperty = CastField<FStructProperty>(Property))
	{
		static const FName NAME_DateTime(TEXT("DateTime"));
#if ENGINE_MAJOR_VERSION == 4
		static const FName NAME_Color(TEXT("Color"));
		static const FName NAME_LinearColor(TEXT("LinearColor"));
#endif
		if (JsonValue->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> Obj = JsonValue->AsObject();
			check(Obj.IsValid()); // should not fail if Type == EJson::Object
			if (!JsonAttributesToUStructWithContainer(Obj->Values, StructProperty->Struct, OutValue, ContainerStruct, Container, CheckFlags & (~CPF_ParmFlags), SkipFlags))
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - FJsonObjectConverter::JsonObjectToUStruct failed for property %s"), *Property->GetNameCPP());
				return false;
			}
		}
		else if (JsonValue->Type == EJson::String && StructProperty->Struct->GetFName() == NAME_LinearColor)
		{
			FLinearColor& ColorOut = *(FLinearColor*)OutValue;
			FString ColorString = JsonValue->AsString();

			FColor IntermediateColor;
			IntermediateColor = FColor::FromHex(ColorString);

			ColorOut = IntermediateColor;
		}
		else if (JsonValue->Type == EJson::String && StructProperty->Struct->GetFName() == NAME_Color)
		{
			FColor& ColorOut = *(FColor*)OutValue;
			FString ColorString = JsonValue->AsString();

			ColorOut = FColor::FromHex(ColorString);
		}
		else if (JsonValue->Type == EJson::String && StructProperty->Struct->GetFName() == NAME_DateTime)
		{
			FString DateString = JsonValue->AsString();
			FDateTime& DateTimeOut = *(FDateTime*)OutValue;
			if (DateString == TEXT("min"))
			{
				// min representable value for our date struct. Actual date may vary by platform (this is used for sorting)
				DateTimeOut = FDateTime::MinValue();
			}
			else if (DateString == TEXT("max"))
			{
				// max representable value for our date struct. Actual date may vary by platform (this is used for sorting)
				DateTimeOut = FDateTime::MaxValue();
			}
			else if (DateString == TEXT("now"))
			{
				// this value's not really meaningful from json serialization (since we don't know timezone) but handle it anyway since we're handling the other keywords
				DateTimeOut = FDateTime::UtcNow();
			}
			else if (FDateTime::ParseIso8601(*DateString, DateTimeOut))
			{
				// ok
			}
			else if (FDateTime::Parse(DateString, DateTimeOut))
			{
				// ok
			}
			else
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable to import FDateTime for property %s"), *Property->GetNameCPP());
				return false;
			}
		}
		else if (JsonValue->Type == EJson::String && StructProperty->Struct->GetCppStructOps() && StructProperty->Struct->GetCppStructOps()->HasImportTextItem())
		{
			UScriptStruct::ICppStructOps* TheCppStructOps = StructProperty->Struct->GetCppStructOps();

			FString ImportTextString = JsonValue->AsString();
			const TCHAR* ImportTextPtr = *ImportTextString;
			if (!TheCppStructOps->ImportTextItem(ImportTextPtr, OutValue, 0x00000000, nullptr, (FOutputDevice*)GWarn))
			{
				// Fall back to trying the tagged property approach if custom ImportTextItem couldn't get it done
				Property->ImportText(ImportTextPtr, OutValue, 0x00000000, nullptr);
			}
		}
		else if (JsonValue->Type == EJson::String)
		{
			FString ImportTextString = JsonValue->AsString();
			const TCHAR* ImportTextPtr = *ImportTextString;
			Property->ImportText(ImportTextPtr, OutValue, 0x00000000, nullptr);
		}
		else
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Attempted to import UStruct from non-object JSON key for property %s"), *Property->GetNameCPP());
			return false;
		}
	}
	else if (FObjectProperty * ObjectProperty = CastField<FObjectProperty>(Property))
	{
		if (JsonValue->Type == EJson::Object)
		{
			UObject* Outer = Cast<UObject>(GetTransientPackage());
			if (ContainerStruct->IsChildOf(UObject::StaticClass()))
			{
				Outer = (UObject*)Container;
			}

			UClass* PropertyClass = ObjectProperty->PropertyClass;
			UObject* createdObj = StaticAllocateObject(PropertyClass, Outer, NAME_None, EObjectFlags::RF_NoFlags, EInternalObjectFlags::None, false);
#if ENGINE_MAJOR_VERSION == 4
			(*PropertyClass->ClassConstructor)(FObjectInitializer(createdObj, PropertyClass->ClassDefaultObject, false, false));
#else
			(*PropertyClass->ClassConstructor)(FObjectInitializer(createdObj, PropertyClass->ClassDefaultObject, EObjectInitializerOptions::None));
#endif		
			ObjectProperty->SetObjectPropertyValue(OutValue, createdObj);

			TSharedPtr<FJsonObject> Obj = JsonValue->AsObject();
			check(Obj.IsValid()); // should not fail if Type == EJson::Object
			if (!JsonAttributesToUStructWithContainer(Obj->Values, ObjectProperty->PropertyClass, createdObj, ObjectProperty->PropertyClass, createdObj, CheckFlags & (~CPF_ParmFlags), SkipFlags))
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - FJsonObjectConverter::JsonObjectToUStruct failed for property %s"), *Property->GetNameCPP());
				return false;
			}
		}
		else if (JsonValue->Type == EJson::String)
		{
			// Default to expect a string for everything else
			if (Property->ImportText(*JsonValue->AsString(), OutValue, 0, NULL) == NULL)
			{
				UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable import property type %s from string value for property %s"), *Property->GetClass()->GetName(), *Property->GetNameCPP());
				return false;
			}
		}
	}
	else
	{
		// Default to expect a string for everything else
		if (Property->ImportText(*JsonValue->AsString(), OutValue, 0, NULL) == NULL)
		{
			UE_LOG(LogJson, Error, TEXT("JsonValueToFProperty - Unable import property type %s from string value for property %s"), *Property->GetClass()->GetName(), *Property->GetNameCPP());
			return false;
		}
	}

	return true;
}


bool UDBJsonBPLibrary::UStructToJsonAttributes(const UStruct* StructDefinition, const void* Struct, TMap< FString, TSharedPtr<FJsonValue> >& OutJsonAttributes, int64 CheckFlags, int64 SkipFlags)
{
	if (SkipFlags == 0)
	{
		// If we have no specified skip flags, skip deprecated, transient and skip serialization by default when writing
		SkipFlags |= CPF_Deprecated | CPF_Transient;
	}

	if (StructDefinition == FJsonObjectWrapper::StaticStruct())
	{
		// Just copy it into the object
		const FJsonObjectWrapper* ProxyObject = (const FJsonObjectWrapper*)Struct;

		if (ProxyObject->JsonObject.IsValid())
		{
			OutJsonAttributes = ProxyObject->JsonObject->Values;
		}
		return true;
	}

	for (TFieldIterator<FProperty> It(StructDefinition); It; ++It)
	{
		FProperty* Property = *It;

		// Check to see if we should ignore this property
		if (CheckFlags != 0 && !Property->HasAnyPropertyFlags(CheckFlags))
		{
			continue;
		}
		if (Property->HasAnyPropertyFlags(SkipFlags))
		{
			continue;
		}

		FString VariableName = FJsonObjectConverter::StandardizeCase(GetShortName(Property));
		const void* Value = Property->ContainerPtrToValuePtr<uint8>(Struct);

		// convert the property to a FJsonValue
		TSharedPtr<FJsonValue> JsonValue = FPropertyToJsonValue(Property, Value, CheckFlags, SkipFlags);
		if (!JsonValue.IsValid())
		{
			auto* PropClass = Property->GetClass();
			UE_LOG(LogJson, Error, TEXT("UStructToJsonObject - Unhandled property type '%s': %s"), *PropClass->GetName(), *Property->GetPathName());
			return false;
		}

		// set the value on the output object
		OutJsonAttributes.Add(VariableName, JsonValue);
	}

	return true;
}


TSharedPtr<FJsonValue> UDBJsonBPLibrary::FPropertyToJsonValue(FProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags)
{
	if (Property->ArrayDim == 1)
	{
		return ConvertScalarFPropertyToJsonValue(Property, Value, CheckFlags, SkipFlags);
	}

	TArray< TSharedPtr<FJsonValue> > Array;
	for (int Index = 0; Index != Property->ArrayDim; ++Index)
	{
		Array.Add(ConvertScalarFPropertyToJsonValue(Property, (char*)Value + Index * Property->ElementSize, CheckFlags, SkipFlags));
	}
	return MakeShared<FJsonValueArray>(Array);
}


// Convert property to JSON, assuming either the property is not an array or the value is an individual array element #1#
TSharedPtr<FJsonValue> UDBJsonBPLibrary::ConvertScalarFPropertyToJsonValue(FProperty* Property, const void* Value, int64 CheckFlags, int64 SkipFlags)
{
	if (FEnumProperty * EnumProperty = CastField<FEnumProperty>(Property))
	{
		// export enums as strings
		UEnum* EnumDef = EnumProperty->GetEnum();
		FString StringValue = EnumDef->GetNameStringByValue(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value));
		return MakeShared<FJsonValueString>(StringValue);
	}
	else if (FNumericProperty * NumericProperty = CastField<FNumericProperty>(Property))
	{
		// see if it's an enum
		UEnum* EnumDef = NumericProperty->GetIntPropertyEnum();
		if (EnumDef != NULL)
		{
			// export enums as strings
			FString StringValue = EnumDef->GetNameStringByValue(NumericProperty->GetSignedIntPropertyValue(Value));
			return MakeShared<FJsonValueString>(StringValue);
		}

		// We want to export numbers as numbers
		if (NumericProperty->IsFloatingPoint())
		{
			return MakeShared<FJsonValueNumber>(NumericProperty->GetFloatingPointPropertyValue(Value));
		}
		else if (NumericProperty->IsInteger())
		{
			return MakeShared<FJsonValueNumber>(NumericProperty->GetSignedIntPropertyValue(Value));
		}

		// fall through to default
	}
	else if (FBoolProperty * BoolProperty = CastField<FBoolProperty>(Property))
	{
		// Export bools as bools
		return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(Value));
	}
	else if (FStrProperty * StringProperty = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(Value));
	}
	else if (FTextProperty * TextProperty = CastField<FTextProperty>(Property))
	{
		return MakeShared<FJsonValueString>(TextProperty->GetPropertyValue(Value).ToString());
	}
	else if (FArrayProperty * ArrayProperty = CastField<FArrayProperty>(Property))
	{
		TArray< TSharedPtr<FJsonValue> > Out;
		FScriptArrayHelper Helper(ArrayProperty, Value);
		for (int32 i = 0, n = Helper.Num(); i < n; ++i)
		{
			TSharedPtr<FJsonValue> Elem = FPropertyToJsonValue(ArrayProperty->Inner, Helper.GetRawPtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags);
			if (Elem.IsValid())
			{
				// add to the array
				Out.Push(Elem);
			}
		}
		return MakeShared<FJsonValueArray>(Out);
	}
	else if (FSetProperty * SetProperty = CastField<FSetProperty>(Property))
	{
		TArray< TSharedPtr<FJsonValue> > Out;
		FScriptSetHelper Helper(SetProperty, Value);
		for (int32 i = 0, n = Helper.Num(); n; ++i)
		{
			if (Helper.IsValidIndex(i))
			{
				TSharedPtr<FJsonValue> Elem = FPropertyToJsonValue(SetProperty->ElementProp, Helper.GetElementPtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags);
				if (Elem.IsValid())
				{
					// add to the array
					Out.Push(Elem);
				}

				--n;
			}
		}
		return MakeShared<FJsonValueArray>(Out);
	}
	else if (FMapProperty * MapProperty = CastField<FMapProperty>(Property))
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

		FScriptMapHelper Helper(MapProperty, Value);
		for (int32 i = 0, n = Helper.Num(); n; ++i)
		{
			if (Helper.IsValidIndex(i))
			{
				TSharedPtr<FJsonValue> KeyElement = FPropertyToJsonValue(MapProperty->KeyProp, Helper.GetKeyPtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags);
				TSharedPtr<FJsonValue> ValueElement = FPropertyToJsonValue(MapProperty->ValueProp, Helper.GetValuePtr(i), CheckFlags & (~CPF_ParmFlags), SkipFlags);
				if (KeyElement.IsValid() && ValueElement.IsValid())
				{
					FString KeyString;
					if (!KeyElement->TryGetString(KeyString))
					{
						MapProperty->KeyProp->ExportTextItem(KeyString, Helper.GetKeyPtr(i), nullptr, nullptr, 0);
						if (KeyString.IsEmpty())
						{
							UE_LOG(LogJson, Error, TEXT("Unable to convert key to string for property %s."), *MapProperty->GetName())
								KeyString = FString::Printf(TEXT("Unparsed Key %d"), i);
						}
					}

					Out->SetField(KeyString, ValueElement);
				}

				--n;
			}
		}

		return MakeShared<FJsonValueObject>(Out);
	}
	else if (FStructProperty * StructProperty = CastField<FStructProperty>(Property))
	{
		UScriptStruct::ICppStructOps* TheCppStructOps = StructProperty->Struct->GetCppStructOps();
		// Intentionally exclude the JSON Object wrapper, which specifically needs to export JSON in an object representation instead of a string
		if (StructProperty->Struct != FJsonObjectWrapper::StaticStruct() && TheCppStructOps && TheCppStructOps->HasExportTextItem())
		{
			FString OutValueStr;
			TheCppStructOps->ExportTextItem(OutValueStr, Value, nullptr, nullptr, 0x00000000, nullptr);
			return MakeShared<FJsonValueString>(OutValueStr);
		}

		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		if (UStructToJsonAttributes(StructProperty->Struct, Value, Out->Values, CheckFlags & (~CPF_ParmFlags), SkipFlags))
		{
			return MakeShared<FJsonValueObject>(Out);
		}
		// fall through to default
	}
	else
	{
		// Default to export as string for everything else
		FString StringValue;
		Property->ExportTextItem(StringValue, Value, NULL, NULL, 0x00000000);
		return MakeShared<FJsonValueString>(StringValue);
	}

	// invalid
	return TSharedPtr<FJsonValue>();
}

