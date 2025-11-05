#pragma once
#include "Core/Public/Object.h"
#include "Global/CurveTypes.h"

/**
 * UCurveLibrary
 * Repository pattern for managing FCurve data
 * Owned by ULevel and serialized with level data
 */
UCLASS()
class UCurveLibrary : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UCurveLibrary, UObject)

public:
	UCurveLibrary();
	virtual ~UCurveLibrary() override;

	/**
	 * Initialize with default preset curves
	 */
	void InitializeDefaults();

	// ========================================
	// CRUD Operations
	// ========================================

	/**
	 * Get curve by name
	 * @param Name Curve name
	 * @return Pointer to curve if found, nullptr otherwise
	 */
	FCurve* GetCurve(const FString& Name);
	const FCurve* GetCurve(const FString& Name) const;

	/**
	 * Add or update curve
	 * @param Name Curve name
	 * @param Curve Curve data
	 * @return True if added/updated successfully
	 */
	bool AddOrUpdateCurve(const FString& Name, const FCurve& Curve);

	/**
	 * Remove curve
	 * @param Name Curve name to remove
	 * @return True if removed successfully
	 */
	bool RemoveCurve(const FString& Name);

	/**
	 * Rename curve
	 * @param OldName Current curve name
	 * @param NewName New curve name
	 * @return True if renamed successfully
	 */
	bool RenameCurve(const FString& OldName, const FString& NewName);

	/**
	 * Check if curve exists
	 * @param Name Curve name
	 * @return True if curve exists
	 */
	bool HasCurve(const FString& Name) const;

	/**
	 * Get all curves
	 * @return Const reference to curve map
	 */
	const TMap<FString, FCurve>& GetAllCurves() const { return Curves; }

	/**
	 * Get all curve names
	 * @return Array of curve names
	 */
	TArray<FString> GetCurveNames() const;

	/**
	 * Get number of curves
	 * @return Curve count
	 */
	int32 GetCurveCount() const { return Curves.Num(); }

	/**
	 * Clear all curves
	 */
	void Clear();

	// ========================================
	// Serialization
	// ========================================

	/**
	 * Serialize curves to/from JSON
	 * @param bLoading True if loading, false if saving
	 * @param InOutHandle JSON object
	 */
	void Serialize(bool bLoading, JSON& InOutHandle);

private:
	/** Curve storage */
	TMap<FString, FCurve> Curves;

	/**
	 * Validate curve name
	 * @param Name Curve name to validate
	 * @return True if valid
	 */
	bool IsValidCurveName(const FString& Name) const;
};
