// Copyright 2021-2022, DearBing. All Rights Reserved.

#include "K2Node_JsonSerialize.h"
#include "Engine/UserDefinedStruct.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "DBJsonBPLibrary.h"
#include "Json.h"

#define LOCTEXT_NAMESPACE "UK2Node_JsonSerialize"

UK2Node_JsonSerialize::UK2Node_JsonSerialize( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	NodeTooltip = LOCTEXT( "NodeTooltip", "Try to parse a structure  a Json string." );
}

void UK2Node_JsonSerialize::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute );

	UEdGraphPin* SuccessPin = CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then );
	SuccessPin->PinFriendlyName = LOCTEXT( "Success Exec pin", "" );

	UEdGraphPin* DataPin = CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TEXT("Structure"));
	DataPin->bDisplayAsMutableRef = true;
	SetPinToolTip( *DataPin, LOCTEXT( "DataPinDescription", "The structure to convert." ) );

	UEdGraphPin* ResultPin = CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_String,  UEdGraphSchema_K2::PN_ReturnValue );
	ResultPin->PinFriendlyName = LOCTEXT( "Out JsonStr", "JsonStr" );
	SetPinToolTip( *ResultPin, LOCTEXT( "ResultPinDescription", "The returned JSON String" ) );

	Super::AllocateDefaultPins();
}

void UK2Node_JsonSerialize::SetPinToolTip( UEdGraphPin& MutatablePin, const FText& PinDescription ) const
{
	MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToText( MutatablePin.PinType ).ToString();

	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>( GetSchema() );
	if ( K2Schema )
	{
		MutatablePin.PinToolTip += TEXT( " " );
		MutatablePin.PinToolTip += K2Schema->GetPinDisplayName( &MutatablePin ).ToString();
	}

	MutatablePin.PinToolTip += FString( TEXT( "\n" ) ) + PinDescription.ToString();
}

void UK2Node_JsonSerialize::RefreshInputPinType()
{
	UEdGraphPin* DataPin = GetDataPin();
	const bool bFillTypeFromConnected = DataPin && ( DataPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard );

	UScriptStruct* InputType = nullptr;
	if ( bFillTypeFromConnected )
	{
		FEdGraphPinType PinType = DataPin->PinType;
		if ( DataPin->LinkedTo.Num() > 0 )
			PinType = DataPin->LinkedTo[ 0 ]->PinType;

		if ( PinType.PinCategory == UEdGraphSchema_K2::PC_Struct )
			InputType = Cast<UScriptStruct>( PinType.PinSubCategoryObject.Get() );
	}

	SetPropertyTypeForStruct( InputType );
}

void UK2Node_JsonSerialize::SetPropertyTypeForStruct( UScriptStruct* StructType )
{
	if ( StructType == GetPropertyTypeForStruct() )
		return;

	UEdGraphPin* DataPin = GetDataPin();
	if ( DataPin->SubPins.Num() > 0 )
		GetSchema()->RecombinePin( DataPin );
		
	DataPin->PinType.PinSubCategoryObject = StructType;
	DataPin->PinType.PinCategory = StructType ?
								   UEdGraphSchema_K2::PC_Struct :
								   UEdGraphSchema_K2::PC_Wildcard;

	CachedNodeTitle.Clear();
}

UScriptStruct* UK2Node_JsonSerialize::GetPropertyTypeForStruct() const
{
	UScriptStruct* DataStructType = (UScriptStruct*)( GetDataPin()->PinType.PinSubCategoryObject.Get() );
	return DataStructType;
}

void UK2Node_JsonSerialize::GetMenuActions( FBlueprintActionDatabaseRegistrar& ActionRegistrar ) const
{
	UClass* ActionKey = GetClass();
	if ( ActionRegistrar.IsOpenForRegistration( ActionKey ) )
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create( GetClass() );
		check( NodeSpawner != nullptr );

		ActionRegistrar.AddBlueprintAction( ActionKey, NodeSpawner );
	}
}

FText UK2Node_JsonSerialize::GetMenuCategory() const
{
	return FText::FromString( TEXT( "DBJson " ) );
}

bool UK2Node_JsonSerialize::IsConnectionDisallowed( const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason ) const
{
	if ( MyPin == GetDataPin() && MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard )
	{
		bool bDisallowed = true;
		if ( OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct )
		{
			if ( UScriptStruct* ConnectionType = Cast<UScriptStruct>( OtherPin->PinType.PinSubCategoryObject.Get() ) )
				bDisallowed = false;
		}
		else if ( OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard )
			bDisallowed = false;

		if ( bDisallowed )
			OutReason = TEXT( "Must be a structure." );

		return bDisallowed;
	}

	return false;
}

FText UK2Node_JsonSerialize::GetTooltipText() const
{
	return NodeTooltip;
}

UEdGraphPin* UK2Node_JsonSerialize::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked( UEdGraphSchema_K2::PN_Then );
	check( Pin->Direction == EGPD_Output );
	return Pin;
}

UEdGraphPin* UK2Node_JsonSerialize::GetDataPin() const
{
	UEdGraphPin* Pin = FindPin(TEXT("Structure"));
	if (Pin == nullptr)
	{
		return nullptr;
	}
	check( Pin->Direction == EGPD_Input );
	return Pin;
}

FText UK2Node_JsonSerialize::GetNodeTitle( ENodeTitleType::Type TitleType ) const
{
	if ( TitleType == ENodeTitleType::MenuTitle )
		return LOCTEXT( "ListViewTitle", "JsonSerialize" );
	
	if ( UEdGraphPin* DataPin = GetDataPin() )
	{
		UScriptStruct* StructType = GetPropertyTypeForStruct();
		if ( !StructType || DataPin->LinkedTo.Num() == 0 )
			return NSLOCTEXT( "K2Node", "JsonSerialize_Title_None", "JsonSerialize Structure" );

		if ( CachedNodeTitle.IsOutOfDate( this ) )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT( "StructName" ), FText::FromName( StructType->GetFName() ) );
			
			FText LocFormat = NSLOCTEXT( "K2Node", "JsonSerialize", "JsonSerialize {StructName}" );
			CachedNodeTitle.SetCachedText( FText::Format( LocFormat, Args ), this );
		}
	}
	else
		return NSLOCTEXT( "K2Node", "JsonSerialize_Title_None", "JsonSerialize Structure" );
	
	return CachedNodeTitle;
}

void UK2Node_JsonSerialize::ExpandNode( FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph )
{
	Super::ExpandNode( CompilerContext, SourceGraph );
	
	const FName StructToJsonFunctionName = GET_FUNCTION_NAME_CHECKED( UDBJsonBPLibrary, JsonSerialize);
	UK2Node_CallFunction* CallStructToJsonFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>( this, SourceGraph );

	CallStructToJsonFunction->FunctionReference.SetExternalMember( StructToJsonFunctionName, UDBJsonBPLibrary::StaticClass() );
	CallStructToJsonFunction->AllocateDefaultPins();

	CompilerContext.MovePinLinksToIntermediate( *GetExecPin(), *( CallStructToJsonFunction->GetExecPin() ) );
	
	UScriptStruct* StructType = GetPropertyTypeForStruct();
	UUserDefinedStruct* UserStructType = Cast<UUserDefinedStruct>( StructType );
	
	UEdGraphPin* StructTypePin = CallStructToJsonFunction->FindPinChecked( TEXT( "StructType" ) );
	if ( UserStructType && UserStructType->PrimaryStruct.IsValid() )
		StructTypePin->DefaultObject = UserStructType->PrimaryStruct.Get();
	else
		StructTypePin->DefaultObject = StructType;

	UEdGraphPin* OriginalDataPin = GetDataPin();
	UEdGraphPin* StructInPin     = CallStructToJsonFunction->FindPinChecked( TEXT( "Struct" ) );

	StructInPin->PinType                      = OriginalDataPin->PinType;
	StructInPin->PinType.PinSubCategoryObject = OriginalDataPin->PinType.PinSubCategoryObject;

	CompilerContext.MovePinLinksToIntermediate( *OriginalDataPin, *StructInPin );

	UEdGraphPin* OriginalReturnPin = FindPinChecked( UEdGraphSchema_K2::PN_ReturnValue );
	UEdGraphPin* FunctionReturnPin = CallStructToJsonFunction->FindPinChecked( UEdGraphSchema_K2::PN_ReturnValue );
	UEdGraphPin* FunctionThenPin   = CallStructToJsonFunction->GetThenPin();
	CompilerContext.MovePinLinksToIntermediate( *GetThenPin(), *FunctionThenPin);
	CompilerContext.MovePinLinksToIntermediate( *OriginalReturnPin, *FunctionReturnPin );
	BreakAllNodeLinks();
}

FSlateIcon UK2Node_JsonSerialize::GetIconAndTint( FLinearColor& OutColor ) const
{
	OutColor = GetNodeTitleColor();
	static FSlateIcon Icon( "EditorStyle", "Kismet.AllClasses.FunctionIcon" );
	return Icon;
}

void UK2Node_JsonSerialize::PostReconstructNode()
{
	Super::PostReconstructNode();
	RefreshInputPinType();
}

void UK2Node_JsonSerialize::EarlyValidation( FCompilerResultsLog& MessageLog ) const
{
	Super::EarlyValidation( MessageLog );
	if ( UEdGraphPin* DataPin = GetDataPin() )
	{
		if ( DataPin->LinkedTo.Num() == 0 )
		{
			MessageLog.Error( *LOCTEXT( "MissingPins", "Missing pins in @@" ).ToString(), this );
			return;
		}
	}
}

void UK2Node_JsonSerialize::NotifyPinConnectionListChanged( UEdGraphPin* Pin )
{
	Super::NotifyPinConnectionListChanged( Pin );
	if ( Pin == GetDataPin() )
		RefreshInputPinType();
}

#undef LOCTEXT_NAMESPACE
