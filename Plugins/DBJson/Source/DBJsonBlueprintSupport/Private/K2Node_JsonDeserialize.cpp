// Copyright 2021-2022, DearBing. All Rights Reserved.

#include "K2Node_JsonDeserialize.h"
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

#define LOCTEXT_NAMESPACE "K2Node_JsonDeserialize"

UK2Node_JsonDeserialize::UK2Node_JsonDeserialize( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	NodeTooltip = LOCTEXT( "NodeTooltip", "Try to parse a JSON String into a structure." );
}

void UK2Node_JsonDeserialize::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute );

	UEdGraphPin* SuccessPin = CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then );
	SuccessPin->PinFriendlyName = LOCTEXT( "Success Exec pin", "Success" );

	UEdGraphPin* FailedPin = CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT("Failed"));
	FailedPin->PinFriendlyName = LOCTEXT( "Failed Exec pin", "Failure" );

	// Input Json str
	UEdGraphPin* DataPin = CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT("JsonStr"));
	SetPinToolTip( *DataPin, LOCTEXT( "DataPinDescription", "The JSON String to convert." ) );

	UEdGraphPin* ResultPin = CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, UEdGraphSchema_K2::PN_ReturnValue );
  //ResultPin->bDisplayAsMutableRef = true;
	ResultPin->PinFriendlyName = LOCTEXT( "JsonStr to Struct Out Struct", "Structure" );
	SetPinToolTip( *ResultPin, LOCTEXT( "ResultPinDescription", "The returned structure, if converted." ) );

	UEdGraphPin* ErroInfoPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, TEXT("ErroInfo"));
	ErroInfoPin->PinFriendlyName = LOCTEXT("Convert Fail Info", "ErroInfo");
	SetPinToolTip(*ErroInfoPin, LOCTEXT("ResultPinDescription", "The returned Erro Info, if converted failed."));
	Super::AllocateDefaultPins();
}

void UK2Node_JsonDeserialize::SetPinToolTip( UEdGraphPin& MutatablePin, const FText& PinDescription ) const
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

void UK2Node_JsonDeserialize::RefreshOutputPinType()
{
	UEdGraphPin* ResultPin = GetResultPin();
	const bool bFillTypeFromConnected = ResultPin && ( ResultPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard );

	UScriptStruct* OutputType = nullptr;
	if ( bFillTypeFromConnected )
	{
		FEdGraphPinType PinType = ResultPin->PinType;
		if ( ResultPin->LinkedTo.Num() > 0 )
			PinType = ResultPin->LinkedTo[ 0 ]->PinType;

		if ( PinType.PinCategory == UEdGraphSchema_K2::PC_Struct )
			OutputType = Cast<UScriptStruct>( PinType.PinSubCategoryObject.Get() );
	}

	SetReturnTypeForStruct( OutputType );
}

void UK2Node_JsonDeserialize::SetReturnTypeForStruct( UScriptStruct* StructType )
{
	if ( StructType == GetReturnTypeForStruct() )
		return;

	UEdGraphPin* ResultPin = GetResultPin();
	if ( ResultPin->SubPins.Num() > 0 )
		GetSchema()->RecombinePin( ResultPin );
		
	ResultPin->PinType.PinSubCategoryObject = StructType;
	ResultPin->PinType.PinCategory = StructType ?
									 UEdGraphSchema_K2::PC_Struct :
									 UEdGraphSchema_K2::PC_Wildcard;

	CachedNodeTitle.Clear();
}

UScriptStruct* UK2Node_JsonDeserialize::GetReturnTypeForStruct() const
{
	UScriptStruct* ReturnStructType = (UScriptStruct*)( GetResultPin()->PinType.PinSubCategoryObject.Get() );
	return ReturnStructType;
}

void UK2Node_JsonDeserialize::GetMenuActions( FBlueprintActionDatabaseRegistrar& ActionRegistrar ) const
{
	UClass* ActionKey = GetClass();
	if ( ActionRegistrar.IsOpenForRegistration( ActionKey ) )
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create( GetClass() );
		check( NodeSpawner != nullptr );

		ActionRegistrar.AddBlueprintAction( ActionKey, NodeSpawner );
	}
}

FText UK2Node_JsonDeserialize::GetMenuCategory() const
{
	return FText::FromString( TEXT( "DBJson" ) );
}

bool UK2Node_JsonDeserialize::IsConnectionDisallowed( const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason ) const
{
	if ( MyPin == GetResultPin() && MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard )
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

FText UK2Node_JsonDeserialize::GetTooltipText() const
{
	return NodeTooltip;
}

UEdGraphPin* UK2Node_JsonDeserialize::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();


	UEdGraphPin* Pin = FindPinChecked( UEdGraphSchema_K2::PN_Then );
	check( Pin->Direction == EGPD_Output );
	return Pin;
}

UEdGraphPin* UK2Node_JsonDeserialize::GetDataPin() const
{
	UEdGraphPin* Pin = FindPinChecked(TEXT("JsonStr"));
	check( Pin->Direction == EGPD_Input );
	return Pin;
}

UEdGraphPin* UK2Node_JsonDeserialize::GetFailedPin() const
{
	UEdGraphPin* Pin = FindPinChecked(TEXT("Failed"));
	check( Pin->Direction == EGPD_Output );
	return Pin;
}

UEdGraphPin* UK2Node_JsonDeserialize::GetResultPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin( UEdGraphSchema_K2::PN_ReturnValue );
	if (Pin == nullptr)
	{
		return nullptr;
	}
	check( Pin->Direction == EGPD_Output );
	return Pin;
}

FText UK2Node_JsonDeserialize::GetNodeTitle( ENodeTitleType::Type TitleType ) const
{
	if ( TitleType == ENodeTitleType::MenuTitle )
		return LOCTEXT( "ListViewTitle", "JsonDeserialize" );
	
	if ( UEdGraphPin* ResultPin = GetResultPin() )
	{
		UScriptStruct* StructType = GetReturnTypeForStruct();
		if ( !StructType || ResultPin->LinkedTo.Num() == 0 )
			return NSLOCTEXT( "K2Node", "JsonDeserialize_Title_None", "JsonDeserialize Structure" );

		if ( CachedNodeTitle.IsOutOfDate( this ) )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT( "StructName" ), FText::FromName( StructType->GetFName() ) );
			
			FText LocFormat = NSLOCTEXT( "K2Node", "JsonDeserialize", "JsonDeserialize {StructName}" );
			CachedNodeTitle.SetCachedText( FText::Format( LocFormat, Args ), this );
		}
	}
	else
		return NSLOCTEXT( "K2Node", "JsonDeserialize_Title_None", "JsonDeserialize Structure" );
	
	return CachedNodeTitle;
}

void UK2Node_JsonDeserialize::ExpandNode( FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph )
{
	Super::ExpandNode( CompilerContext, SourceGraph );
	
	const FName StructFromJsonStrFunctionName = GET_FUNCTION_NAME_CHECKED( UDBJsonBPLibrary, JsonDeserialize);
	UK2Node_CallFunction* CallStructFromJsonFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>( this, SourceGraph );

	CallStructFromJsonFunction->FunctionReference.SetExternalMember( StructFromJsonStrFunctionName, UDBJsonBPLibrary::StaticClass() );
	CallStructFromJsonFunction->AllocateDefaultPins();

	CompilerContext.MovePinLinksToIntermediate( *GetExecPin(), *( CallStructFromJsonFunction->GetExecPin() ) );

	UScriptStruct* StructType = GetReturnTypeForStruct();
	UUserDefinedStruct* UserStructType = Cast<UUserDefinedStruct>( StructType );

	UEdGraphPin* StructTypePin = CallStructFromJsonFunction->FindPinChecked( TEXT( "StructType" ) );
	if ( UserStructType && UserStructType->PrimaryStruct.IsValid() )
		StructTypePin->DefaultObject = UserStructType->PrimaryStruct.Get();
	else
		StructTypePin->DefaultObject = StructType;

	UEdGraphPin* DataInPin = CallStructFromJsonFunction->FindPinChecked( TEXT( "JsonStr" ) );
	CompilerContext.MovePinLinksToIntermediate( *GetDataPin(), *DataInPin );
	
	UEdGraphPin* OriginalReturnPin = FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	UEdGraphPin* FunctionOutStructPin = CallStructFromJsonFunction->FindPinChecked(TEXT("OutStruct"));
	UEdGraphPin* FunctionReturnPin = CallStructFromJsonFunction->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	UEdGraphPin* FunctionThenPin = CallStructFromJsonFunction->GetThenPin();

	UEdGraphPin* OriginalErroInfoPin = FindPinChecked(TEXT("ErroInfo"));
	UEdGraphPin* ErroInfoPin   = CallStructFromJsonFunction->FindPinChecked(TEXT("ErroInfo"));;
	CompilerContext.MovePinLinksToIntermediate(*OriginalErroInfoPin, *ErroInfoPin);
	
	FunctionOutStructPin->PinType                      = OriginalReturnPin->PinType;
	FunctionOutStructPin->PinType.PinSubCategoryObject = OriginalReturnPin->PinType.PinSubCategoryObject;

	UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>( this, SourceGraph );
	BranchNode->AllocateDefaultPins();

	FunctionThenPin->MakeLinkTo(BranchNode->GetExecPin());
	FunctionReturnPin->MakeLinkTo(BranchNode->GetConditionPin());
	
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *(BranchNode->GetThenPin()));
	CompilerContext.MovePinLinksToIntermediate(*GetFailedPin(), *(BranchNode->GetElsePin()));
	CompilerContext.MovePinLinksToIntermediate(*OriginalReturnPin, *FunctionOutStructPin);
	BreakAllNodeLinks();
}

FSlateIcon UK2Node_JsonDeserialize::GetIconAndTint( FLinearColor& OutColor ) const
{
	OutColor = GetNodeTitleColor();
	static FSlateIcon Icon( "EditorStyle", "Kismet.AllClasses.FunctionIcon" );
	return Icon;
}

void UK2Node_JsonDeserialize::PostReconstructNode()
{
	Super::PostReconstructNode();
	RefreshOutputPinType();
}

void UK2Node_JsonDeserialize::EarlyValidation( FCompilerResultsLog& MessageLog ) const
{
	Super::EarlyValidation( MessageLog );
	if ( UEdGraphPin* ResultPin = GetResultPin() )
	{
		if ( ResultPin->LinkedTo.Num() == 0 )
		{
			MessageLog.Error( *LOCTEXT( "MissingPins", "Missing pins in @@" ).ToString(), this );
			return;
		}
	}
}

void UK2Node_JsonDeserialize::NotifyPinConnectionListChanged( UEdGraphPin* Pin )
{
	Super::NotifyPinConnectionListChanged( Pin );
	if ( Pin == GetResultPin() )
		RefreshOutputPinType();
}

#undef LOCTEXT_NAMESPACE
