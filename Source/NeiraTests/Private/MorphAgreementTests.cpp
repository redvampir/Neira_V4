#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FMorphAgreement.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAgreement_FeminineWord_InflectsByCases,
    "Neira.MorphAgreement.FeminineWord_InflectsByCases",
    NEIRA_TEST_FLAGS)
bool FMorphAgreement_FeminineWord_InflectsByCases::RunTest(const FString& Parameters)
{
    const FEntityTargetForms Forms = FMorphAgreement::BuildEntityTargetForms(TEXT("машина"));
    TestEqual(TEXT("Nominative"), Forms.Nominative, TEXT("машина"));
    TestEqual(TEXT("Prepositional"), Forms.Prepositional, TEXT("машине"));
    TestEqual(TEXT("Instrumental"), Forms.Instrumental, TEXT("машиной"));
    TestFalse(TEXT("Fallback не нужен"), Forms.bUsedFallback);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAgreement_Multiword_FallbackQuoted,
    "Neira.MorphAgreement.Multiword_FallbackQuoted",
    NEIRA_TEST_FLAGS)
bool FMorphAgreement_Multiword_FallbackQuoted::RunTest(const FString& Parameters)
{
    const FEntityTargetForms Forms = FMorphAgreement::BuildEntityTargetForms(TEXT("база данных"));
    TestTrue(TEXT("Fallback включен"), Forms.bUsedFallback);
    TestEqual(TEXT("Prepositional fallback"), Forms.Prepositional, TEXT("«база данных»"));
    TestEqual(TEXT("Instrumental fallback"), Forms.Instrumental, TEXT("«база данных»"));
    return true;
}

