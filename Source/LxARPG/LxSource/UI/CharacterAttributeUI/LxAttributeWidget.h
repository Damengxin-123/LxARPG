#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxAttributeWidget.generated.h"

class ALxBaseCharacter;
class ULxCharacterAttributeComponent;
class ULxUITextData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeItemListChanged, const TArray<ULxUITextData*>&, ItemList);

/**
 * @brief 瑙掕壊灞炴€х晫闈㈤€昏緫瀵硅薄
 *
 * C++ 灞傝礋璐ｄ粠瑙掕壊灞炴€х粍浠朵腑鏁寸悊灞炴€у垪琛ㄦ暟鎹紝
 * 骞堕€氳繃浜嬩欢濮旀墭閫氱煡钃濆浘瀛愮被鍒锋柊 ListView/TileView 绛夌晫闈㈡帶浠躲€? */
UCLASS()
class LXARPG_API ULxAttributeWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 鍒濆鍖栬鑹插睘鎬х晫闈㈤€昏緫瀵硅薄銆?	 */
	virtual void InitializeUIComponents() override;

	/**
	 * @brief 閲嶇疆瑙掕壊灞炴€х晫闈㈢殑鏄剧ず鏁版嵁銆?	 *
	 * 娓呯┖褰撳墠鍒楄〃骞跺箍鎾┖鏁版嵁缁欑晫闈㈠眰銆?	 */
	virtual void ResetUIComponents() override;

	/**
	 * @brief 鏍规嵁褰撳墠瑙掕壊鍒锋柊灞炴€х晫闈㈡暟鎹€?	 *
	 * @param PlayerCharacter 褰撳墠闇€瑕佹樉绀哄睘鎬т俊鎭殑瑙掕壊瀵硅薄銆?	 */
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	/** 褰撹鑹插睘鎬у彂鐢熷彉鍖栨椂閲嶅缓灞炴€ф樉绀哄垪琛ㄣ€?*/
	UFUNCTION()
	void HandleAttributeChanged();

	/** 骞挎挱褰撳墠瑙掕壊灞炴€ф潯鐩垪琛紝渚涜摑鍥句紶閫掔粰鍒楄〃鎺т欢銆?*/
	UPROPERTY(BlueprintAssignable, Category="Character Attribute")
	FOnAttributeItemListChanged OnAttributeItemListChanged;

protected:
	/** 钃濆浘瀹炵幇鐨勫睘鎬у垪琛ㄥ埛鏂颁簨浠躲€?*/
	UFUNCTION(BlueprintImplementableEvent, Category="Character Attribute")
	void ReceiveAttributeItemListChanged(const TArray<ULxUITextData*>& ItemList);

private:
	/** 鏋勫缓瑙掕壊灞炴€ф樉绀哄垪琛ㄣ€?*/
	void ShowRoleProperties();

	/** 灏嗘寚瀹氬垎绫讳笅鐨勫睘鎬ц拷鍔犲埌鏄剧ず鍒楄〃涓€?*/
	void AppendAttributeGroup(TArray<ULxUITextData*>& OutItemList, const TArray<const FLxAttributeData*>& InAttributes, ELxAttributeType InAttributeType, const FString& InTitle, bool& bIsDarkColor) const;

private:
	/** 褰撳墠瑙掕壊鐨勫睘鎬х粍浠躲€?*/
	UPROPERTY()
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent = nullptr;
};

