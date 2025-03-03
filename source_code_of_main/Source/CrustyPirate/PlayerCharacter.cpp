#include "PlayerCharacter.h"

#include "Enemy.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	AttackCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollisionBox"));
	AttackCollisionBox->SetupAttachment(RootComponent);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	OnAttackOverrideEndDelegate.BindUObject(this, &APlayerCharacter::OnAttackOverrideAnimEnd);

	AttackCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::AttackBoxOverlapBegin);
	EnableAttackCollisionBox(false);

	MyGameInstance = Cast<UCrustyPirateGameInstance>(GetGameInstance());
	if (MyGameInstance)
	{
		HitPoints = MyGameInstance->PlayerHP;

		if (MyGameInstance->IsDoubleJumpUnlocked)
		{
			UnlockDoubleJump();
		}
	}

	if (PlayerHUDClass)
	{
		PlayerHUDWidget = CreateWidget<UPlayerHUD>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerHUDClass);
		if (PlayerHUDWidget)
		{
			PlayerHUDWidget->AddToPlayerScreen();

			PlayerHUDWidget->SetHP(HitPoints);
			PlayerHUDWidget->SetDiamonds(MyGameInstance->CollectedDiamondCount);
			PlayerHUDWidget->SetLevel(MyGameInstance->CurrentLevelIndex);
		}
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up input action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::JumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::JumpEnded);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &APlayerCharacter::JumpEnded);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::Attack);

		EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Started, this, &APlayerCharacter::QuitGame);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	float MoveActionValue = Value.Get<float>();

	if (IsAlive && CanMove && !IsStunned)
	{
		FVector Direction = FVector(1.0f, 0.0f, 0.0f);
		AddMovementInput(Direction, MoveActionValue);
		UpdateDirection(MoveActionValue);
	}
}

void APlayerCharacter::UpdateDirection(float MoveDirection)
{
	FRotator CurrentRotation = Controller->GetControlRotation();

	if (MoveDirection < 0.0f)
	{
		if (CurrentRotation.Yaw != 180.0f)
		{
			Controller->SetControlRotation(FRotator(CurrentRotation.Pitch, 180.0f, CurrentRotation.Roll));
		}
	}
	else if (MoveDirection > 0.0f)
	{
		if (CurrentRotation.Yaw != 0.0f)
		{
			Controller->SetControlRotation(FRotator(CurrentRotation.Pitch, 0.0f, CurrentRotation.Roll));
		}
	}
}

void APlayerCharacter::JumpStarted(const FInputActionValue& Value)
{
	if (IsAlive && CanMove && !IsStunned)
	{
		Jump();
	}
}

void APlayerCharacter::JumpEnded(const FInputActionValue& Value)
{
	StopJumping();
}

void APlayerCharacter::Attack(const FInputActionValue& Value)
{
	if (IsAlive && CanAttack && !IsStunned)
	{
		CanAttack = false;
		CanMove = false;

		//EnableAttackCollisionBox(true);

		GetAnimInstance()->PlayAnimationOverride(AttackAnimSequence, FName("DefaultSlot"), 1.0f, 0.0f, OnAttackOverrideEndDelegate);
	}
}

void APlayerCharacter::OnAttackOverrideAnimEnd(bool Completed)
{
	if (IsActive && IsAlive)
	{
		CanAttack = true;
		CanMove = true;
	}

	//EnableAttackCollisionBox(false);
}

void APlayerCharacter::AttackBoxOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemy* Enemy = Cast<AEnemy>(OtherActor);

	if (Enemy)
	{
		Enemy->TakeDamage(AttackDamage, AttackStunDuration);
	}
}

void APlayerCharacter::EnableAttackCollisionBox(bool Enabled)
{
	if (Enabled)
	{
		AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AttackCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}
	else
	{
		AttackCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AttackCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	}
}

void APlayerCharacter::TakeDamage(int DamageAmount, float StunDuration)
{
	if (!IsAlive) return;
	if (!IsActive) return;

	Stun(StunDuration);

	UpdateHP(HitPoints - DamageAmount);

	if (HitPoints <= 0)
	{
		// Player is dead
		UpdateHP(0);

		IsAlive = false;
		CanMove = false;
		CanAttack = false;

		GetAnimInstance()->JumpToNode(FName("JumpDie"), FName("CaptainStateMachine"));
		EnableAttackCollisionBox(false);

		float RestartDelay = 3.0f;
		GetWorldTimerManager().SetTimer(RestartTimer, this, &APlayerCharacter::OnRestartTimerTimeout, 1.0f, false, RestartDelay);

	}
	else
	{
		// Player is still alive
		GetAnimInstance()->JumpToNode(FName("JumpTakeHit"), FName("CaptainStateMachine"));
	}
}

void APlayerCharacter::UpdateHP(int NewHP)
{
	HitPoints = NewHP;
	MyGameInstance->SetPlayerHP(HitPoints);
	PlayerHUDWidget->SetHP(HitPoints);
}

void APlayerCharacter::Stun(float DurationInSeconds)
{
	IsStunned = true;

	bool IsTimerAlreadyActive = GetWorldTimerManager().IsTimerActive(StunTimer);
	if (IsTimerAlreadyActive)
	{
		GetWorldTimerManager().ClearTimer(StunTimer);
	}

	GetWorldTimerManager().SetTimer(StunTimer, this, &APlayerCharacter::OnStunTimerTimeout, 1.0f, false, DurationInSeconds);

	GetAnimInstance()->StopAllAnimationOverrides();
	EnableAttackCollisionBox(false);
}

void APlayerCharacter::OnStunTimerTimeout()
{
	IsStunned = false;
}

void APlayerCharacter::CollectItem(CollectableType ItemType)
{
	UGameplayStatics::PlaySound2D(GetWorld(), ItemPickupSound);

	switch (ItemType)
	{
		case CollectableType::HealthPotion:
		{
			int HealAmount = 25;
			UpdateHP(HitPoints + HealAmount);
		}break;

		case CollectableType::Diamond:
		{
			MyGameInstance->AddDiamond(1);
			PlayerHUDWidget->SetDiamonds(MyGameInstance->CollectedDiamondCount);
		}break;

		case CollectableType::DoubleJumpUpgrade:
		{
			if (!MyGameInstance->IsDoubleJumpUnlocked)
			{
				MyGameInstance->IsDoubleJumpUnlocked = true;
				UnlockDoubleJump();
			}
		}break;

		default:
		{
		}break;
	}
}

void APlayerCharacter::UnlockDoubleJump()
{
	JumpMaxCount = 2;
}

void APlayerCharacter::OnRestartTimerTimeout()
{
	MyGameInstance->RestartGame();
}

void APlayerCharacter::Deactivate()
{
	if (IsActive)
	{
		IsActive = false;
		CanAttack = false;
		CanMove = false;

		GetCharacterMovement()->StopMovementImmediately();
	}
}

void APlayerCharacter::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, false);
}