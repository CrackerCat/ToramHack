static inline void log(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    AddNPCMessage.Call(CreateMonoString("HACK"), CreateMonoString(buffer));
}

static int (*o_checkMobReaction)(...);

static int my_checkMobReaction(Il2CppObject *thiz, int guard, int avoid) {
    if (thiz == nullptr || perfectHits) return 0;
    return o_checkMobReaction(thiz, guard, avoid);
}

static bool (*o_checkPercent)(...);

static bool my_checkPercent(Il2CppObject *thiz, int max, int per) {
    if (thiz == nullptr) return false;
    if (perfectHits) return true;
    return o_checkPercent(thiz, max, per);
}

static bool (*o_CheckPercent1)(...);

static bool my_CheckPercent1(int percent) {
    if (alwaysCrit) return true;
    return o_CheckPercent1(percent);
}

static Il2CppObject *hackTarget = nullptr;

static bool (*o_targetToEnemy)(...);

static bool my_targetToEnemy(Il2CppObject *thiz, Il2CppObject *manager) {
    if (thiz == nullptr || manager == nullptr) return false;

    Property<Il2CppObject *> transform = LoadClass(manager).GetPropertyByName("transform");

    hackTarget = transform[manager].Get();

    return o_targetToEnemy(thiz, manager);
}

static bool (*o_ChackCorrectHit)(...);

static bool
my_ChackCorrectHit(Il2CppObject *thiz, Il2CppObject *status, int hit, bool isFlash, bool *correct) {
    if (thiz == nullptr || status == nullptr || correct == nullptr) return false;

    if (perfectHits) {
        *correct = false;
        return true;
    }

    return o_ChackCorrectHit(thiz, status, hit, isFlash, correct);
}

static Il2CppObject *(*o_CreateSkillData)(...);

static Il2CppObject *my_CreateSkillData(Il2CppObject *reader, int version) {
    auto data = o_CreateSkillData(reader, version);

    EqLimit[data].Set(1573119);

    return data;
}

static float (*o_CalcStablePercent)(...);

static float my_CalcStablePercent(int stable) {
    if (perfectHits) return 1;
    return o_CalcStablePercent(stable);
}

static float (*o_CalcMagicStablePercent)(...);

static float my_CalcMagicStablePercent(int stable) {
    if (perfectHits) return 1;
    return o_CalcMagicStablePercent(stable);
}

static bool (*o_checkAbnormalPercent)(...);

static bool
my_checkAbnormalPercent(Il2CppObject *thiz, int abnormalType, int per, Il2CppObject *playerAction,
                        Il2CppObject *mobAction, float *addResistTime) {
    if (thiz == nullptr || playerAction == nullptr || mobAction == nullptr ||
        addResistTime == nullptr)
        return false;

    Property<bool> IsRange = LoadClass(thiz).GetPropertyByName("IsRange");

    if (damageHack > 0 && IsRange[thiz].Get()) {
        *addResistTime = 0;
        return false;
    }

    return o_checkAbnormalPercent(thiz, abnormalType, per, playerAction, mobAction, addResistTime);
}

static void (*o_OnSubmit)(...);

static void my_OnSubmit(Il2CppObject *thiz) {
    if (thiz == nullptr) return;

    AddNPCMessage.SetInstance(thiz);
    inputLabel.SetInstance(thiz);
    mText.SetInstance(inputLabel.Get());

    auto text = mText.Get();
    auto str = text->str();

    if (str.starts_with('.')) {

        std::vector<std::string> vector;

        for (const auto &item: std::views::split(str, ' ')) {
            vector.emplace_back(item.begin(), item.end());
        }

        if (!vector.empty()) {
            if (".i" == vector[0] && vector.size() == 1) {
                isInvincible = !isInvincible;
                log("Invecible: %d", isInvincible);
            } else if (".a" == vector[0] && vector.size() == 2) {
                attackSpeed = std::stoi(vector[1]);
                log("Attack speed: %d", attackSpeed);
            } else if (".d" == vector[0] && vector.size() == 2) {
                damageHack = std::stoi(vector[1]);
                log("Damage mult: %d", damageHack);
            } else if (".p" == vector[0] && vector.size() == 1) {
                perfectHits = !perfectHits;
                log("Perfect hits: %d", perfectHits);
            } else if (".c" == vector[0] && vector.size() == 1) {
                alwaysCrit = !alwaysCrit;
                log("Always critical: %d", alwaysCrit);
            } else {
                log("Command not found :(");
            }
        }

        writeConfig();
        return;
    }

    return o_OnSubmit(thiz);
}

static void (*o_CalcDamage)(Il2CppObject *, Il2CppObject *, Il2CppObject *);

static void
my_CalcDamage(Il2CppObject *thiz, Il2CppObject *actarAction, Il2CppObject *targetAction) {

    if (thiz == nullptr || actarAction == nullptr || targetAction == nullptr) return;

    bool isTargetPlayer =
            std::string_view(targetAction->klass->name).find("Player") != std::string_view::npos;

    if (isInvincible && isTargetPlayer) return;

    Property<bool> IsRange = LoadClass(thiz).GetPropertyByName("IsRange");

    bool isActarPlayer =
            std::string_view(actarAction->klass->name).find("Player") != std::string_view::npos;

    if (IsRange[thiz].Get() && damageHack > 1 && isActarPlayer) {

        for (int i = 0; i < damageHack; ++i) {
            o_CalcDamage(thiz, actarAction, targetAction);
        }

        return;
    }

    return o_CalcDamage(thiz, actarAction, targetAction);
}

static int (*o_GetMotionSpeed)(...);

static int my_GetMotionSpeed(Il2CppObject *thiz, int aspd) {
    if (attackSpeed > 0) aspd = attackSpeed;
    return o_GetMotionSpeed(thiz, aspd);
}

static int (*o_GetNextAtkTime)(...);

static int my_GetNextAtkTime(Il2CppObject *thiz, int aspd) {
    if (attackSpeed > 0) aspd = attackSpeed;
    return o_GetNextAtkTime(thiz, aspd);
}

static bool (*o_checkCriticalPercent)(...);

static bool
my_checkCriticalPercent(Il2CppObject *thiz, int criticalParcent, Il2CppObject *mobAction) {
    if (thiz == nullptr || mobAction == nullptr) return false;
    return alwaysCrit || o_checkCriticalPercent(thiz, criticalParcent, mobAction);
}

static void (*o_LateUpdate)(Il2CppObject *);

static void my_LateUpdate(Il2CppObject *thiz) {
    SkipMode[thiz].Set(4);
    return o_LateUpdate(thiz);
}