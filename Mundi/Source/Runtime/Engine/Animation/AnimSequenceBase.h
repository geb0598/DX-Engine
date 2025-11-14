#pragma once
#include "AnimationAsset.h"

class UAnimDataModel;

/**
 * UAnimSequenceBase
 * 시퀀스 기반 애니메이션의 베이스 클래스
 */
class UAnimSequenceBase : public UAnimationAsset
{
public:
	DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)

	UAnimSequenceBase();
	virtual ~UAnimSequenceBase() override;

	// 데이터 모델 접근
	virtual UAnimDataModel* GetDataModel() const { return DataModel; }
	virtual void SetDataModel(UAnimDataModel* InDataModel) { DataModel = InDataModel; }

protected:
	UAnimDataModel* DataModel;
};
