#include <android/log.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <thread>
#include <ranges>
#include "zygisk.hpp"
#include "cJSON.h"
#include "shadowhook.h"
#include "Loading.hpp"
#include "UserSettings/GlobalSettings.hpp"
#include "Class.hpp"
#include "Field.hpp"
#include "Method.hpp"
#include "Property.hpp"
#include "Operators.hpp"
#include "BasicMonoStructures.hpp"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ToramHack", __VA_ARGS__)

static std::string path;

using namespace BNM;
using namespace BNM::IL2CPP;
using namespace BNM::Structures::Mono;
using namespace BNM::Structures::Unity;
using namespace BNM::Utils;

bool perfectHits = true;
bool isInvincible = true;
bool alwaysCrit = true;
int attackSpeed = 10000;
int damageHack = 0;
int animationSpeed = 0;

static void writeFile(char *str) {
    FILE *file = fopen(path.c_str(), "w");

    if (!file) return;

    fprintf(file, "%s", str);

    fclose(file);
}

static std::string readFile() {
    std::string str;

    FILE *file = fopen(path.c_str(), "r");

    if (!file) return str;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    str.resize(size);
    fread(str.data(), 1, size, file);
    fclose(file);

    return str;
}

static void writeConfig() {
    cJSON *json = cJSON_CreateObject();

    if (!json) return;

    cJSON_AddBoolToObject(json, "Invincible", isInvincible);
    cJSON_AddBoolToObject(json, "PerfectHits", perfectHits);
    cJSON_AddBoolToObject(json, "AlwaysCritical", alwaysCrit);
    cJSON_AddNumberToObject(json, "AttackSpeed", attackSpeed);
    cJSON_AddNumberToObject(json, "damageHack", damageHack);
    cJSON_AddNumberToObject(json, "animationSpeed", animationSpeed);

    char *jsonString = cJSON_Print(json);

    if (!jsonString) return;

    writeFile(jsonString);

    cJSON_Delete(json);
    free(jsonString);
}

Method<void> AddNPCMessage;
Field<Il2CppObject *> inputLabel;
Field<String *> mText;
Property<int> EqLimit;
Property<uint8_t> SkipMode;

#include "hooks.h"

static void il2cppHack() {
    while (!IsLoaded()) sleep(1);

    LOGD("Starting to hack...");

    Class AbnormalData("", "AbnormalData");
    auto AbnormalDataCtor = AbnormalData.GetMethod(".ctor");
    HOOK(AbnormalDataCtor.GetOffset(), my_AbnormalDataCtor, o_AbnormalDataCtor);

    Class SkillDamageData("", "SkillDamageData");
    Method<void> SetAbnormalType = SkillDamageData.GetMethod("SetAbnormalType", 2);
    Method<void> SetAbnormalType1 = SkillDamageData.GetMethod("SetAbnormalType", 3);
    HOOK(SetAbnormalType.GetOffset(), my_SetAbnormalType, o_SetAbnormalType);
    HOOK(SetAbnormalType1.GetOffset(), my_SetAbnormalType1, o_SetAbnormalType1);

    Class TakePlayer("", "TakePlayer");
    Method<int> ParamCheck = TakePlayer.GetMethod("ParamCheck");
    HOOK(ParamCheck.GetOffset(), my_ParamCheck, o_ParamCheck);

    Class AbnormalStateManager("", "AbnormalStateManager");
    Method<bool> IsInvincibility = AbnormalStateManager.GetMethod("IsInvincibility");
    HOOK(IsInvincibility.GetOffset(), my_IsInvincibility, o_IsInvincibility);
    Method<float> GetAnbormalStateResistTime = AbnormalStateManager.GetMethod(
            "GetAnbormalStateResistTime");
    HOOK(GetAnbormalStateResistTime.GetOffset(), my_GetAnbormalStateResistTime,
         o_GetAnbormalStateResistTime);

    Class FieldScriptManager("", "FieldScriptManager");
    SkipMode = FieldScriptManager.GetProperty("SkipMode");
    auto LateUpdate = FieldScriptManager.GetMethod("LateUpdate");
    HOOK(LateUpdate, my_LateUpdate, o_LateUpdate);

    Class EnemyMobActionManagerBase("", "EnemyMobActionManagerBase");
    Method<void> SetSystemInvincible = EnemyMobActionManagerBase.GetMethod("SetSystemInvincible");
    HOOK(SetSystemInvincible.GetOffset(), my_SetSystemInvincible, o_SetSystemInvincible);
    Method<float> checkAddAbnormalResistTime = EnemyMobActionManagerBase.GetMethod(
            "checkAddAbnormalResistTime");
    HOOK(checkAddAbnormalResistTime.GetOffset(), my_checkAddAbnormalResistTime,
         o_checkAddAbnormalResistTime);

    Class MathUtil("", "MathUtil");
    auto CheckPercent1 = MathUtil.GetMethod("CheckPercent");
    HOOK(CheckPercent1, my_CheckPercent1, o_CheckPercent1);

    Class PlayerBattleManager("", "PlayerBattleManager");
    auto targetToEnemy = PlayerBattleManager.GetMethod("targetToEnemy");
    HOOK(targetToEnemy, my_targetToEnemy, o_targetToEnemy);

    Class SkillMasterData("", "SkillMasterData");
    EqLimit = SkillMasterData.GetProperty("EqLimit");
    auto CreateSkillData = SkillMasterData.GetMethod("CreateSkillData");
    HOOK(CreateSkillData, my_CreateSkillData, o_CreateSkillData);

    Class PlayerAttackBase("", "PlayerAttackBase");
    auto checkMobReaction = PlayerAttackBase.GetMethod("checkMobReaction");
    HOOK(checkMobReaction, my_checkMobReaction, o_checkMobReaction);
    auto checkCriticalPercent = PlayerAttackBase.GetMethod("checkCriticalPercent");
    HOOK(checkCriticalPercent, my_checkCriticalPercent, o_checkCriticalPercent);
    auto checkAbnormalPercent = PlayerAttackBase.GetMethod("checkAbnormalPercent");
    HOOK(checkAbnormalPercent, my_checkAbnormalPercent, o_checkAbnormalPercent);
    auto ChackCorrectHit = PlayerAttackBase.GetMethod("ChackCorrectHit");
    HOOK(ChackCorrectHit, my_ChackCorrectHit, o_ChackCorrectHit);

    Class ChatWindow("", "ChatWindow");
    auto OnSubmit = ChatWindow.GetMethod("OnSubmit", 0);
    HOOK(OnSubmit, my_OnSubmit, o_OnSubmit);

    AddNPCMessage = ChatWindow.GetMethod("AddNPCMessage", 2);

    inputLabel = ChatWindow.GetField("inputLabel");

    Class UILabel("", "UILabel");
    mText = UILabel.GetField("mText");

    Class SkillActionBase("", "SkillActionBase");
    auto checkPercent = SkillActionBase.GetMethod("checkPercent");
    HOOK(checkPercent, my_checkPercent, o_checkPercent);
    auto CalcDamage = SkillActionBase.GetMethod("CalcDamage", 2);
    HOOK(CalcDamage, my_CalcDamage, o_CalcDamage);
    auto CalcStablePercent = SkillActionBase.GetMethod("CalcStablePercent");
    HOOK(CalcStablePercent, my_CalcStablePercent, o_CalcStablePercent);
    auto CalcMagicStablePercent = SkillActionBase.GetMethod("CalcMagicStablePercent");
    HOOK(CalcMagicStablePercent, my_CalcMagicStablePercent, o_CalcMagicStablePercent);

    Class PlayerBattleStatus("", "PlayerBattleStatus");
    auto GetMotionSpeed = PlayerBattleStatus.GetMethod("GetMotionSpeed", 1);
    HOOK(GetMotionSpeed, my_GetMotionSpeed, o_GetMotionSpeed);

    auto GetNextAtkTime = PlayerBattleStatus.GetMethod("GetNextAtkTime", 1);
    HOOK(GetNextAtkTime, my_GetNextAtkTime, o_GetNextAtkTime);

    LOGD("End hack :D");
}

static void readConfig() {
    std::string jsonStr = readFile();

    if (jsonStr.empty()) return;

    cJSON *json = cJSON_ParseWithLength(jsonStr.c_str(), jsonStr.size());

    const cJSON *j_Invincible = cJSON_GetObjectItemCaseSensitive(json, "Invincible");
    const cJSON *j_PerfectHits = cJSON_GetObjectItemCaseSensitive(json, "PerfectHits");
    const cJSON *j_AlwaysCritical = cJSON_GetObjectItemCaseSensitive(json, "AlwaysCritical");
    const cJSON *j_AttackSpeed = cJSON_GetObjectItemCaseSensitive(json, "AttackSpeed");
    const cJSON *j_damageHack = cJSON_GetObjectItemCaseSensitive(json, "damageHack");
    const cJSON *j_animationSpeed = cJSON_GetObjectItemCaseSensitive(json, "animationSpeed");

    if (j_Invincible) {
        isInvincible = cJSON_IsTrue(j_Invincible);
    }

    if (j_PerfectHits) {
        perfectHits = cJSON_IsTrue(j_PerfectHits);
    }

    if (j_AlwaysCritical) {
        alwaysCrit = cJSON_IsTrue(j_AlwaysCritical);
    }

    if (j_AttackSpeed) {
        attackSpeed = j_AttackSpeed->valueint;
    }

    if (j_damageHack) {
        damageHack = j_damageHack->valueint;
    }

    if (j_animationSpeed) {
        animationSpeed = j_animationSpeed->valueint;
    }

    cJSON_Delete(json);
}

static void start_hack(const std::string &str) {
    readConfig();

    shadowhook_init(SHADOWHOOK_MODE_UNIQUE, true);

    LOGD("Thread started! Config file: %s", str.c_str());

    path = str;

    sleep(5);

    void *handle = BNM_dlopen("libil2cpp.so");

    LOGD("Il2cpp handle: %p", handle);

    BNM::Loading::AddOnLoadedEvent(il2cppHack);

    BNM::Loading::TryLoadByDlfcnHandle(handle);

    BNM::Loading::TrySetupByUsersFinder();
}

class ToramHack : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        auto dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        if (!dir) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        bool isToram = std::string_view(dir).ends_with("/com.asobimo.toramonline");

        env->ReleaseStringUTFChars(args->app_data_dir, dir);

        if (!isToram) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);

        appDir = dir;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (appDir.empty()) return;

        std::thread(start_hack, appDir + "/data.json").detach();
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override {
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;
    std::string appDir;
};

REGISTER_ZYGISK_MODULE(ToramHack)