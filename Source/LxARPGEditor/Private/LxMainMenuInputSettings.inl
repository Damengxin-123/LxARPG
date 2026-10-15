// 此文件在 LxMenuLayout 命名空间内引入，复用菜单资产工具的编译、备份与图节点辅助函数。

/** 读取项目实际配置的输入表，避免通过资产路径猜测创建另一份预设。 */
UDataTable* GetConfiguredInputSettingsTable()
{
	UClass* ManagerClass = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
	const ULxGameDataTablesManager* Manager = ManagerClass ? ManagerClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
	UDataTable* Table = Manager ? Manager->m_pInputActionInfoTableConfig.Get() : nullptr;
	return Table && Table->GetRowStruct() == FLxInputActionInfo::StaticStruct() ? Table : nullptr;
}

/** 在第一次输入页迁移前备份真实配置表，重复执行保留最早的备份。 */
bool BackupInputSettingsTable(UDataTable* Table)
{
	const FString Filename = FPackageName::LongPackageNameToFilename(Table->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	if (!IFileManager::Get().FileExists(*Filename)) return true;
	FString Relative = Filename; FPaths::MakePathRelativeTo(Relative, *FPaths::ProjectContentDir());
	const FString Target = FPaths::ProjectSavedDir() / TEXT("MainMenuLayout/BeforeInputSettings/Content") / Relative;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Target), true);
	return IFileManager::Get().FileExists(*Target) || IFileManager::Get().Copy(*Target, *Filename, false, true) == COPY_OK;
}

/** 为缺失的新默认操作选择简短中文行名，不覆盖任何已有行。 */
FName MakeInputSettingsRowName(const UDataTable* Table, const TCHAR* BaseName)
{
	FName Candidate(BaseName);
	for (int32 Index = 1; Table->GetRowMap().Contains(Candidate); ++Index)
		Candidate = FName(*FString::Printf(TEXT("%s%d"), BaseName, Index));
	return Candidate;
}

/** 只补缺失的瞄准和冲刺，并明确锁定视角及连续轴；其余预设字段原样保留。 */
bool EnsureDefaultInputSettingsTable(bool bApply)
{
	UDataTable* Table = GetConfiguredInputSettingsTable();
	if (!Table)
	{
		UE_LOG(LogTemp, Error, TEXT("InputSettings: 项目未配置有效的玩家操控输入配置表"));
		return false;
	}
	bool bHasAim = false;
	bool bHasSprint = false;
	TArray<TPair<FName, FLxInputActionInfo>> LockedRows;
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FLxInputActionInfo& Info = *reinterpret_cast<const FLxInputActionInfo*>(Pair.Value);
		bHasAim |= Info.InputActionID == ELxInputActionID::Aim;
		bHasSprint |= Info.InputActionID == ELxInputActionID::Sprint;
		if (Info.bCanUserRemap && (Info.InputActionID == ELxInputActionID::LookX || Info.InputActionID == ELxInputActionID::LookY || Info.DefaultKey.IsAnalog()))
		{
			FLxInputActionInfo Locked = Info; Locked.bCanUserRemap = false;
			LockedRows.Emplace(Pair.Key, Locked);
		}
	}
	if (bHasAim && bHasSprint && LockedRows.IsEmpty()) return true;
	if (!bApply)
	{
		UE_LOG(LogTemp, Error, TEXT("InputSettings: 默认表未迁移，瞄准=%s 冲刺=%s 待锁定轴行=%d 表=%s"),
			bHasAim ? TEXT("存在") : TEXT("缺失"), bHasSprint ? TEXT("存在") : TEXT("缺失"), LockedRows.Num(), *Table->GetPathName());
		return false;
	}
	if (!BackupInputSettingsTable(Table)) return false;
	Table->Modify();
	for (const TPair<FName, FLxInputActionInfo>& Pair : LockedRows) Table->AddRow(Pair.Key, Pair.Value);
	for (ELxInputActionID ActionID : {ELxInputActionID::Aim, ELxInputActionID::Sprint})
	{
		if ((ActionID == ELxInputActionID::Aim && bHasAim) || (ActionID == ELxInputActionID::Sprint && bHasSprint)) continue;
		const TCHAR* Name = ActionID == ELxInputActionID::Aim ? TEXT("瞄准") : TEXT("冲刺");
		FLxInputActionInfo Info;
		Info.InputActionID = ActionID;
		Info.DisplayName = FText::FromString(Name);
		Info.DefaultKey = ActionID == ELxInputActionID::Aim ? EKeys::RightMouseButton : EKeys::LeftShift;
		Info.ValueType = EInputActionValueType::Boolean;
		Info.InteractionType = ELxInputInteractionType::PressAndRelease;
		Info.ValueDirection = ELxInputValueAxial::None;
		Info.ValueMagnification = 1.f;
		Table->AddRow(MakeInputSettingsRowName(Table, Name), Info);
	}
	Table->HandleDataTableChanged(); Table->MarkPackageDirty();
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(Table->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	if (!UPackage::SavePackage(Table->GetPackage(), Table, *Filename, Args)) return false;
	UE_LOG(LogTemp, Display, TEXT("InputSettings: 默认表已补全，新增瞄准=%d 新增冲刺=%d 锁定轴行=%d 表=%s"),
		!bHasAim, !bHasSprint, LockedRows.Num(), *Table->GetPathName());
	return EnsureDefaultInputSettingsTable(false);
}

/** 返回与设置蓝图同目录的中文按键行资产路径。 */
FString GetInputRowPath()
{
	return FPackageName::GetLongPackagePath(GetSettingsPath()) + TEXT("/输入按键行");
}

/** 检查已保存事件是否直接调用指定业务接口，避免迁移重复连接。 */
bool HasInputSettingsCall(UWidgetBlueprint* Blueprint, const TCHAR* Function)
{
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
		for (UEdGraphNode* Entry : Graph->Nodes)
			if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Entry))
				if (CallNode->FunctionReference.GetMemberName() == Function) return true;
	return false;
}

/** 找到已有父类通知事件，已有蓝图执行链必须保留。 */
UK2Node_Event* FindInputSettingsEvent(UWidgetBlueprint* Blueprint, const TCHAR* Name)
{
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
		for (UEdGraphNode* Entry : Graph->Nodes)
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Entry))
				if (EventNode->EventReference.GetMemberName() == Name) return EventNode;
	return nullptr;
}

/** 在原事件执行链之前插入一个业务调用，随后继续全部已有连线。 */
void PrependInputSettingsCall(UK2Node_Event* Source, UK2Node_CallFunction* CallNode)
{
	UEdGraphPin* Exec = Source->FindPinChecked(TEXT("then"));
	const TArray<UEdGraphPin*> Existing = Exec->LinkedTo;
	Exec->BreakAllPinLinks();
	Connect(Exec, CallNode->GetExecPin());
	for (UEdGraphPin* Pin : Existing) Connect(CallNode->GetThenPin(), Pin);
}

/** 首次创建可编辑按键行；已有自定义行蓝图不重建。 */
bool EnsureInputRowBlueprint(bool bApply, UWidgetBlueprint*& OutRow)
{
	OutRow = LoadOrCreate(*GetInputRowPath(), ULxInputSettingRowWidget::StaticClass(), bApply);
	if (!OutRow || !OutRow->ParentClass || !OutRow->ParentClass->IsChildOf(ULxInputSettingRowWidget::StaticClass())) return false;
	if (bApply && IsEmpty(OutRow))
	{
		USizeBox* Root = Make<USizeBox>(OutRow, TEXT("按键行尺寸"));
		Root->SetHeightOverride(42.f);
		OutRow->WidgetTree->RootWidget = Root;
		UHorizontalBox* Cells = Make<UHorizontalBox>(OutRow, TEXT("按键行内容"));
		Root->AddChild(Cells);
		UTextBlock* Name = Label(OutRow, TEXT("操作名称"), TEXT("操作"), 16);
		UTextBlock* Default = Label(OutRow, TEXT("默认按键"), TEXT("默认按键"), 16);
		UButton* Current = Button(OutRow, TEXT("修改按键"), TEXT("当前按键"), 16);
		Name->SetJustification(ETextJustify::Left);
		for (UWidget* Cell : TArray<UWidget*>{Name, Default, Current})
		{
			UHorizontalBoxSlot* Slot = Cells->AddChildToHorizontalBox(Cell);
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(4, 3));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		Bind(OutRow, Name, TEXT("Text"), TEXT("GetActionDisplayName"));
		Bind(OutRow, Default, TEXT("Text"), TEXT("GetDefaultKeyText"));
		Bind(OutRow, CastChecked<UTextBlock>(Current->GetContent()), TEXT("Text"), TEXT("GetCurrentKeyText"));
		if (UWidgetBlueprint* Settings = LoadObject<UWidgetBlueprint>(nullptr, *GetSettingsPath()))
		{
			if (UButton* Theme = Cast<UButton>(Settings->WidgetTree->FindWidget(TEXT("应用设置"))))
			{
				Current->SetStyle(Theme->GetStyle()); Current->SetBackgroundColor(Theme->GetBackgroundColor());
			}
			if (UTextBlock* ThemeText = Cast<UTextBlock>(Settings->WidgetTree->FindWidget(TEXT("音量标题"))))
				for (UTextBlock* Text : {Name, Default, CastChecked<UTextBlock>(Current->GetContent())})
				{
					FSlateFontInfo Font = ThemeText->GetFont(); Font.Size = 16;
					Text->SetFont(Font); Text->SetColorAndOpacity(ThemeText->GetColorAndOpacity());
				}
		}
		if (!Compile(OutRow)) return false;
		Click(OutRow, NewGraph(OutRow), TEXT("修改按键"), TEXT("RequestKeyCapture"), 0);
		if (!Compile(OutRow) || !Save(OutRow)) return false;
	}
	return Verify(OutRow, ULxInputSettingRowWidget::StaticClass());
}

/** 保存并复核设置蓝图类默认值，编译后再次读取以防止生成类重建丢失配置。 */
bool EnsureInputRowDefault(UWidgetBlueprint* Blueprint, UClass* RowClass, bool bApply)
{
	if (!Blueprint || !Blueprint->GeneratedClass || !RowClass) return false;
	ULxSettingsWidget* Defaults = Cast<ULxSettingsWidget>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!Defaults) return false;
	if (Defaults->InputRowWidgetClass.Get() == RowClass) return true;
	if (!bApply || !Backup(Blueprint, TEXT("BeforeInputSettings"))) return false;
	Defaults->Modify(); Defaults->InputRowWidgetClass = RowClass;
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	if (!Compile(Blueprint)) return false;
	const ULxSettingsWidget* CompiledDefaults = Cast<ULxSettingsWidget>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!CompiledDefaults || CompiledDefaults->InputRowWidgetClass.Get() != RowClass)
	{
		UE_LOG(LogTemp, Error, TEXT("InputSettings: 编译后按键行类默认值未保留 %s"), *Blueprint->GetPathName());
		return false;
	}
	return Save(Blueprint);
}

/** 同步已有主菜单与暂停蓝图内的空设置模板，防止旧嵌入实例遮蔽新类默认值。 */
bool SynchronizeInputSettingsHosts(bool bApply)
{
	const FString PausePath = GetDefault<ULxMainMenuSettings>()->PauseMenuWidgetClass.ToSoftObjectPath().GetLongPackageName();
	for (const FString& Path : TArray<FString>{GetMenuPath(), PausePath})
	{
		if (Path.IsEmpty()) continue;
		UWidgetBlueprint* Host = LoadObject<UWidgetBlueprint>(nullptr, *Path);
		if (!Host && Path == PausePath) continue;
		if (!Host || !Host->WidgetTree) return false;
		TArray<ULxSettingsWidget*> MissingTemplates;
		Host->WidgetTree->ForEachWidgetAndDescendants([&MissingTemplates](UWidget* Widget)
		{
			if (ULxSettingsWidget* Settings = Cast<ULxSettingsWidget>(Widget))
			{
				if (!Settings->InputRowWidgetClass) MissingTemplates.Add(Settings);
			}
		});
		if (MissingTemplates.IsEmpty()) continue;
		if (!bApply || !Backup(Host, TEXT("BeforeInputSettings"))) return false;
		Host->Modify(); Host->WidgetTree->Modify();
		for (ULxSettingsWidget* Settings : MissingTemplates)
		{
			const ULxSettingsWidget* Defaults = Settings->GetClass()->GetDefaultObject<ULxSettingsWidget>();
			if (!Defaults || !Defaults->InputRowWidgetClass) return false;
			Settings->Modify(); Settings->InputRowWidgetClass = Defaults->InputRowWidgetClass;
		}
		if (!Compile(Host) || !Save(Host)) return false;
		UE_LOG(LogTemp, Display, TEXT("InputSettings: 已同步宿主蓝图内 %d 个空设置模板 %s"), MissingTemplates.Num(), *Host->GetPathName());
	}
	return true;
}

/** 只同步新增输入控件的文字主题；已有控件树、事件、字号及布局均保持原样。 */
bool EnsureInputSettingsTextStyles(UWidgetBlueprint* Settings, UWidgetBlueprint* InputRow, bool bApply)
{
	if (!Settings || !Settings->WidgetTree || !InputRow || !InputRow->WidgetTree) return false;
	const UTextBlock* ButtonTheme = Cast<UTextBlock>(Settings->WidgetTree->FindWidget(TEXT("应用设置文字")));
	const UTextBlock* LabelTheme = Cast<UTextBlock>(Settings->WidgetTree->FindWidget(TEXT("画质标题")));
	if (!ButtonTheme || !LabelTheme) return false;
	TArray<TPair<UTextBlock*, const UTextBlock*>> SettingsTexts;
	TArray<TPair<UTextBlock*, const UTextBlock*>> RowTexts;
	for (const TCHAR* Name : {TEXT("常规设置文字"), TEXT("输入设置文字"), TEXT("恢复默认按键文字")})
	{
		UTextBlock* Text = Cast<UTextBlock>(Settings->WidgetTree->FindWidget(Name));
		if (!Text) return false;
		SettingsTexts.Emplace(Text, ButtonTheme);
	}
	for (const TCHAR* Name : {TEXT("输入设置说明"), TEXT("操作列标题"), TEXT("默认列标题"), TEXT("当前列标题"), TEXT("输入设置提示")})
	{
		UTextBlock* Text = Cast<UTextBlock>(Settings->WidgetTree->FindWidget(Name));
		if (!Text) return false;
		SettingsTexts.Emplace(Text, LabelTheme);
	}
	for (const TCHAR* Name : {TEXT("操作名称"), TEXT("默认按键"), TEXT("修改按键文字")})
	{
		UTextBlock* Text = Cast<UTextBlock>(InputRow->WidgetTree->FindWidget(Name));
		if (!Text) return false;
		RowTexts.Emplace(Text, FString(Name) == TEXT("修改按键文字") ? ButtonTheme : LabelTheme);
	}
	/** 保留局部字号，继承现有设置页的字体资产、字形和配色。 */
	const auto MatchesTheme = [](const TPair<UTextBlock*, const UTextBlock*>& Pair)
	{
		FSlateFontInfo Desired = Pair.Value->GetFont(); Desired.Size = Pair.Key->GetFont().Size;
		return Pair.Key->GetFont() == Desired && Pair.Key->GetColorAndOpacity() == Pair.Value->GetColorAndOpacity();
	};
	SettingsTexts.RemoveAll(MatchesTheme); RowTexts.RemoveAll(MatchesTheme);
	if (SettingsTexts.IsEmpty() && RowTexts.IsEmpty()) return true;
	if (!bApply) return false;
	if ((!SettingsTexts.IsEmpty() && !Backup(Settings, TEXT("BeforeInputTextTheme")))
		|| (!RowTexts.IsEmpty() && !Backup(InputRow, TEXT("BeforeInputTextTheme")))) return false;
	/** 每个文字控件仅写入主题属性，不改变绑定或交互。 */
	const auto ApplyTheme = [](const TPair<UTextBlock*, const UTextBlock*>& Pair)
	{
		FSlateFontInfo Desired = Pair.Value->GetFont(); Desired.Size = Pair.Key->GetFont().Size;
		Pair.Key->Modify(); Pair.Key->SetFont(Desired); Pair.Key->SetColorAndOpacity(Pair.Value->GetColorAndOpacity());
	};
	for (const auto& Pair : SettingsTexts) ApplyTheme(Pair);
	for (const auto& Pair : RowTexts) ApplyTheme(Pair);
	if ((!SettingsTexts.IsEmpty() && (!Compile(Settings) || !Save(Settings)))
		|| (!RowTexts.IsEmpty() && (!Compile(InputRow) || !Save(InputRow)))) return false;
	UE_LOG(LogTemp, Display, TEXT("InputSettings: 已同步输入页文字主题，设置控件=%d 按键行控件=%d"), SettingsTexts.Num(), RowTexts.Num());
	return true;
}

/** 校验输入页控件和业务调用，供命令行独立验收或重复迁移。 */
bool VerifyInputSettingsBlueprint(UWidgetBlueprint* Blueprint, UWidgetBlueprint* RowBlueprint)
{
	if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->GeneratedClass || !RowBlueprint) return false;
	if (!Verify(Blueprint, ULxSettingsWidget::StaticClass())) return false;
	const ULxSettingsWidget* Defaults = Cast<ULxSettingsWidget>(Blueprint->GeneratedClass->GetDefaultObject());
	const UWidgetSwitcher* Pages = Cast<UWidgetSwitcher>(Blueprint->WidgetTree->FindWidget(TEXT("设置分页")));
	if (!Defaults || Defaults->InputRowWidgetClass.Get() != RowBlueprint->GeneratedClass.Get() || !Pages || Pages->GetChildrenCount() != 2) return false;
	for (const TCHAR* Name : {TEXT("常规设置"), TEXT("输入设置"), TEXT("输入按键列表"), TEXT("恢复默认按键"), TEXT("输入设置提示")})
		if (!Blueprint->WidgetTree->FindWidget(Name)) return false;
	for (const TCHAR* Function : {TEXT("RegisterInputSettingsList"), TEXT("OpenGeneralSettingsPage"), TEXT("OpenInputSettingsPage"), TEXT("RestoreDefaultInputSettings")})
		if (!HasInputSettingsCall(Blueprint, Function)) return false;
	return true;
}

/** 原位补充设置分页，保留原有画面声音控件、主题、事件和主菜单逻辑。 */
bool RunInputSettings(bool bApply)
{
	if (!EnsureDefaultInputSettingsTable(bApply)) return false;
	UWidgetBlueprint* RowBlueprint = nullptr;
	if (!EnsureInputRowBlueprint(bApply, RowBlueprint)) return false;
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *GetSettingsPath());
	if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(ULxSettingsWidget::StaticClass()) || !Blueprint->WidgetTree) return false;
	if (!bApply) return EnsureInputSettingsTextStyles(Blueprint, RowBlueprint, false)
		&& VerifyInputSettingsBlueprint(Blueprint, RowBlueprint) && SynchronizeInputSettingsHosts(false);
	if (Blueprint->WidgetTree->FindWidget(TEXT("设置分页")))
		return EnsureInputSettingsTextStyles(Blueprint, RowBlueprint, true)
			&& EnsureInputRowDefault(Blueprint, RowBlueprint->GeneratedClass.Get(), true)
			&& VerifyInputSettingsBlueprint(Blueprint, RowBlueprint) && SynchronizeInputSettingsHosts(true);
	UVerticalBox* Root = Cast<UVerticalBox>(Blueprint->WidgetTree->FindWidget(TEXT("设置内容")));
	UWidget* Title = Blueprint->WidgetTree->FindWidget(TEXT("设置标题"));
	UWidget* Footer = Blueprint->WidgetTree->FindWidget(TEXT("设置操作"));
	if (!Root || Root != Blueprint->WidgetTree->RootWidget || !Title || Title->GetParent() != Root || !Footer || Footer->GetParent() != Root)
	{
		UE_LOG(LogTemp, Error, TEXT("InputSettings: 设置蓝图结构与现有布局不符，未修改 %s"), *Blueprint->GetPathName());
		return false;
	}
	for (const TCHAR* Name : {TEXT("设置分页标签"), TEXT("常规设置内容"), TEXT("输入设置内容"), TEXT("输入按键列表"), TEXT("常规设置"), TEXT("输入设置"), TEXT("恢复默认按键")})
		if (Blueprint->WidgetTree->FindWidget(Name)) return false;
	if (!Backup(Blueprint, TEXT("BeforeInputSettings"))) return false;
	Blueprint->Modify(); Blueprint->WidgetTree->Modify();
	UVerticalBox* General = Make<UVerticalBox>(Blueprint, TEXT("常规设置内容"));
	const TArray<UWidget*> Existing = Root->GetAllChildren();
	for (UWidget* Child : Existing)
	{
		if (Child == Title || Child == Footer) continue;
		const UVerticalBoxSlot* Old = Cast<UVerticalBoxSlot>(Child->Slot);
		if (!Old) return false;
		const FMargin Padding = Old->GetPadding();
		const FSlateChildSize Size = Old->GetSize();
		const EHorizontalAlignment Horizontal = Old->GetHorizontalAlignment();
		const EVerticalAlignment Vertical = Old->GetVerticalAlignment();
		Root->RemoveChild(Child);
		UVerticalBoxSlot* Slot = General->AddChildToVerticalBox(Child);
		Slot->SetPadding(Padding); Slot->SetSize(Size);
		Slot->SetHorizontalAlignment(Horizontal); Slot->SetVerticalAlignment(Vertical);
	}
	UButton* GeneralTab = Button(Blueprint, TEXT("常规设置"), TEXT("常规设置"), 18);
	UButton* InputTab = Button(Blueprint, TEXT("输入设置"), TEXT("输入设置"), 18);
	UHorizontalBox* Tabs = Actions(Blueprint, TEXT("设置分页标签"), {GeneralTab, InputTab});
	Row(Root, Tabs, 16);
	Root->ShiftChild(1, Tabs);
	UWidgetSwitcher* Pages = Make<UWidgetSwitcher>(Blueprint, TEXT("设置分页"));
	Pages->AddChild(General);
	UVerticalBox* Input = Make<UVerticalBox>(Blueprint, TEXT("输入设置内容"));
	Pages->AddChild(Input);
	UTextBlock* Help = Label(Blueprint, TEXT("输入设置说明"), TEXT("点击当前按键后，使用键盘按键、鼠标按钮或单方向滚轮。按 Esc 取消。"), 14);
	Help->SetAutoWrapText(true); Help->SetJustification(ETextJustify::Left);
	Row(Input, Help, 10);
	Row(Input, Actions(Blueprint, TEXT("按键列表标题"), {Label(Blueprint, TEXT("操作列标题"), TEXT("操作"), 16), Label(Blueprint, TEXT("默认列标题"), TEXT("预设按键"), 16), Label(Blueprint, TEXT("当前列标题"), TEXT("当前按键"), 16)}), 8);
	UScrollBox* List = Make<UScrollBox>(Blueprint, TEXT("输入按键列表"));
	USizeBox* ListSize = Make<USizeBox>(Blueprint, TEXT("输入列表尺寸"));
	ListSize->SetHeightOverride(320.f); ListSize->AddChild(List); Row(Input, ListSize, 12);
	Row(Input, Button(Blueprint, TEXT("恢复默认按键"), TEXT("恢复默认按键"), 17), 8);
	UTextBlock* Status = Label(Blueprint, TEXT("输入设置提示"), TEXT(""), 14);
	Status->SetAutoWrapText(true); Status->SetJustification(ETextJustify::Left);
	Bind(Blueprint, Status, TEXT("Text"), TEXT("GetInputStatusText")); Row(Input, Status, 8);
	Row(Root, Pages, 12); Root->ShiftChild(2, Pages);
	Pages->SetActiveWidgetIndex(0);
	// 新增行和分页按钮继承已有应用按钮的主题，避免混入模板的深色默认样式。
	if (UButton* Theme = Cast<UButton>(Blueprint->WidgetTree->FindWidget(TEXT("应用设置"))))
		for (UButton* ButtonWidget : {GeneralTab, InputTab, CastChecked<UButton>(Blueprint->WidgetTree->FindWidget(TEXT("恢复默认按键")))})
		{
			ButtonWidget->SetStyle(Theme->GetStyle()); ButtonWidget->SetBackgroundColor(Theme->GetBackgroundColor());
		}
	if (!Compile(Blueprint)) return false;
	UEdGraph* Graph = Blueprint->UbergraphPages.IsEmpty() ? NewGraph(Blueprint) : Blueprint->UbergraphPages[0].Get();
	Click(Blueprint, Graph, TEXT("常规设置"), TEXT("OpenGeneralSettingsPage"), 9000);
	Click(Blueprint, Graph, TEXT("输入设置"), TEXT("OpenInputSettingsPage"), 9300);
	Click(Blueprint, Graph, TEXT("恢复默认按键"), TEXT("RestoreDefaultInputSettings"), 9600);
	UK2Node_Event* Construct = FindInputSettingsEvent(Blueprint, TEXT("Construct"));
	if (!Construct) Construct = Event(Graph, UUserWidget::StaticClass(), TEXT("Construct"), 10000);
	UK2Node_CallFunction* Register = Call(Construct->GetGraph(), Blueprint->ParentClass, TEXT("RegisterInputSettingsList"), 360, 10000);
	Connect(Widget(Construct->GetGraph(), TEXT("输入按键列表"), 360, 10200)->GetValuePin(), ValueInput(Register));
	PrependInputSettingsCall(Construct, Register);
	UK2Node_Event* Changed = FindInputSettingsEvent(Blueprint, TEXT("ReceiveSettingsPageChanged"));
	if (!Changed) Changed = Event(Graph, Blueprint->ParentClass, TEXT("ReceiveSettingsPageChanged"), 10600);
	UK2Node_CallFunction* SetPage = WidgetCall(Changed->GetGraph(), TEXT("设置分页"), UWidgetSwitcher::StaticClass(), TEXT("SetActiveWidgetIndex"), 360, 10600);
	Connect(Call(Changed->GetGraph(), Blueprint->ParentClass, TEXT("GetSettingsPageIndex"), 360, 10800)->GetReturnValuePin(), ValueInput(SetPage));
	PrependInputSettingsCall(Changed, SetPage);
	if (!Compile(Blueprint)) return false;
	if (!EnsureInputSettingsTextStyles(Blueprint, RowBlueprint, true)
		|| !EnsureInputRowDefault(Blueprint, RowBlueprint->GeneratedClass.Get(), true)
		|| !VerifyInputSettingsBlueprint(Blueprint, RowBlueprint) || !Save(Blueprint)) return false;
	UE_LOG(LogTemp, Display, TEXT("InputSettings: 已补充输入设置分页 %s，按键行=%s"), *Blueprint->GetPathName(), *RowBlueprint->GetPathName());
	return SynchronizeInputSettingsHosts(true);
}
