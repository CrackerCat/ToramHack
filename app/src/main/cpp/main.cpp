#include <android/log.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include "zygisk.hpp"
#include "strutil.h"
#include "json.hpp"
#include "shadowhook.h"
#include <BNM/Loading.hpp>
#include <BNM/UserSettings/GlobalSettings.hpp>
#include <BNM/Class.hpp>
#include <BNM/Field.hpp>
#include <BNM/Method.hpp>
#include <BNM/Property.hpp>
#include <BNM/Operators.hpp>
#include <BNM/BasicMonoStructures.hpp>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ToramHack", __VA_ARGS__)

static std::string path;
static nlohmann::json json;

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

static void writeConfig() {
    std::ofstream ofs(path, std::ios::trunc);

    json["Invincible"] = isInvincible;
    json["PerfectHits"] = perfectHits;
    json["AlwaysCritical"] = alwaysCrit;
    json["AttackSpeed"] = attackSpeed;

    ofs << json;
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

    Class AbnormalStateManager("", "AbnormalStateManager");
    Method<bool> IsInvincibility = AbnormalStateManager.GetMethod("IsInvincibility");
    HOOK(IsInvincibility.GetOffset(), my_IsInvincibility, o_IsInvincibility);

    Class FieldScriptManager("", "FieldScriptManager");
    SkipMode = FieldScriptManager.GetProperty("SkipMode");
    auto LateUpdate = FieldScriptManager.GetMethod("LateUpdate");
    HOOK(LateUpdate, my_LateUpdate, o_LateUpdate);

    Class EnemyMobActionManagerBase("", "EnemyMobActionManagerBase");
    Method<void> SetSystemInvincible = EnemyMobActionManagerBase.GetMethod("SetSystemInvincible");
    HOOK(SetSystemInvincible.GetOffset(), my_SetSystemInvincible, o_SetSystemInvincible);

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
    if (!std::filesystem::exists(path)) return;

    std::ifstream ifs(path);
    json = nlohmann::json::parse(ifs, nullptr, false, true);

    if (json.contains("Invincible") && json["Invincible"].is_boolean()) {
        isInvincible = json["Invincible"].get<bool>();
    } else {
        LOGD("Error reading isInvincible");
    }

    if (json.contains("PerfectHits") && json["PerfectHits"].is_boolean()) {
        perfectHits = json["PerfectHits"].get<bool>();
    } else {
        LOGD("Error reading perfectHits");
    }

    if (json.contains("AlwaysCritical") && json["AlwaysCritical"].is_boolean()) {
        alwaysCrit = json["AlwaysCritical"].get<bool>();
    } else {
        LOGD("Error reading alwaysCrit");
    }

    if (json.contains("AttackSpeed") && json["AttackSpeed"].is_number_integer()) {
        attackSpeed = json["AttackSpeed"].get<int>();
    } else {
        LOGD("Error reading attackSpeed");
    }
}

static void start_hack(const std::string &str) {
    shadowhook_init(SHADOWHOOK_MODE_UNIQUE, true);

    LOGD("Thread started! Config file: %s", str.c_str());

    path = str;

    sleep(5);

    void *handle = BNM_dlopen("libil2cpp.so");

    LOGD("Il2cpp handle: %p", handle);

    BNM::Loading::TryLoadByDlfcnHandle(handle);

    BNM::Loading::AddOnLoadedEvent(il2cppHack);

    BNM::Loading::TrySetupByUsersFinder();

    readConfig();
}

class ToramHack : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        const char *dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        if (dir != nullptr) {

            bool isToram = std::string_view(dir).ends_with("/com.asobimo.toramonline");

            env->ReleaseStringUTFChars(args->app_data_dir, dir);

            if (isToram) {
                appDir = dir;
                api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);
                return;
            }
        }

        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
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