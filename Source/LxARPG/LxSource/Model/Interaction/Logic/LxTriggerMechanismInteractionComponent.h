#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxTriggerMechanismInteractionComponent.generated.h"

class ULxPlayerInteractionComponent;

/** 鏈哄叧鐘舵€佹敼鍙樹簨浠躲€?*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxMechanismStateChanged, ELxMechanismState, NewState);

/** 鏈哄叧绫诲瀷浜や簰缁勪欢锛岃礋璐ｈЕ鍙戞満鍏炽€佺淮鎶ゆ満鍏崇姸鎬侊紝骞舵寜鐘舵€佹彁渚涗氦浜掓彁绀烘枃鏈爣绛俱€?*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType, DisplayName="Trigger Mechanism Interaction Component")
class LXARPG_API ULxTriggerMechanismInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	ULxTriggerMechanismInteractionComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 瑙﹀彂鏈哄叧銆傞粯璁や細鍦ㄥ叧闂拰寮€鍚姸鎬佷箣闂村垏鎹紝涓嶅彲寮€鍚姸鎬佷笉浼氳Е鍙戞垚鍔熴€?*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interaction|Mechanism", DisplayName="Trigger Mechanism")
	bool TriggerMechanism(ULxPlayerInteractionComponent* PlayerInteractionComponent);
	virtual bool TriggerMechanism_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent);

	/** 璁剧疆鏈哄叧鐘舵€侊紝骞跺箍鎾満鍏崇姸鎬佹敼鍙樹簨浠躲€?*/
	UFUNCTION(BlueprintCallable, Category="Interaction|Mechanism", DisplayName="Set Mechanism State")
	void SetMechanismState(ELxMechanismState InMechanismState);

	/** 鑾峰彇褰撳墠鏈哄叧鐘舵€併€?*/
	UFUNCTION(BlueprintCallable, Category="Interaction|Mechanism", DisplayName="Get Mechanism State")
	ELxMechanismState GetMechanismState() const { return MechanismState; }

	virtual FGameplayTag GetPromptTextTag() const override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent) override;

	/** 鏈哄叧鐘舵€佹敼鍙樻椂瑙﹀彂锛屼緵钃濆浘鍝嶅簲闂ㄣ€佸紑鍏崇瓑琛ㄧ幇銆?*/
	UPROPERTY(BlueprintAssignable, Category="Interaction|Mechanism", DisplayName="Mechanism State Changed")
	FOnLxMechanismStateChanged OnMechanismStateChanged;

protected:
	/** 褰撳墠鏈哄叧鐘舵€併€?*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MechanismState, Category="Interaction|Mechanism", DisplayName="Mechanism State")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 鍚勬満鍏崇姸鎬佸搴旂殑浜や簰鎻愮ず鏂囨湰鏍囩锛屼緥濡傚叧闂姸鎬佹樉绀衡€滃紑闂ㄢ€濓紝寮€鍚姸鎬佹樉绀衡€滃叧闂ㄢ€濄€?*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Mechanism", DisplayName="Mechanism State Prompt Text Tags")
	TMap<ELxMechanismState, FGameplayTag> MechanismStatePromptTextTags;

private:
	UFUNCTION()
	void OnRep_MechanismState();
};
