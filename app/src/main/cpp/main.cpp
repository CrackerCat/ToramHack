#include <android/log.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include <ranges>
#include "zygisk.hpp"
#include "json.hpp"
#include "BNM.hpp"
#include "KittyInclude.hpp"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "ToramHack", __VA_ARGS__)

static std::string path;
static nlohmann::json json;

using namespace BNM;
using namespace BNM::IL2CPP;
using namespace BNM::Structures::Mono;
using namespace BNM::Structures::Unity;
using namespace BNM::Utils;

typedef bool (*T_CheckRangeHit)(Il2CppObject *, Il2CppObject *, Il2CppObject *, float);

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
Method<Il2CppObject *> FindPlayer;
Field<Il2CppObject *> inputLabel;
Field<monoString *> mText;
Property<int> EqLimit;
Property<uint8_t> SkipMode;
Property<bool> SystemInvincible;

#include "hooks.h"

static void il2cppHack() {
    while (!IsLoaded()) sleep(1);

    LOGD("Starting to hack...");

    LoadClass FieldScriptManager("", "FieldScriptManager");
    SkipMode = FieldScriptManager.GetPropertyByName("SkipMode");
    auto LateUpdate = FieldScriptManager.GetMethodByName("LateUpdate");
    HOOK(LateUpdate, my_LateUpdate, o_LateUpdate);

    LoadClass EnemyMobActionManagerBase("", "EnemyMobActionManagerBase");
    SystemInvincible = EnemyMobActionManagerBase.GetPropertyByName("SystemInvincible");
    MemoryPatch::createWithHex(SystemInvincible.getter.GetOffset(), "00008052C0035FD6").Modify();

    LoadClass MathUtil("", "MathUtil");
    auto CheckPercent1 = MathUtil.GetMethodByName("CheckPercent");
    HOOK(CheckPercent1, my_CheckPercent1, o_CheckPercent1);

    LoadClass PlayerBattleManager("", "PlayerBattleManager");
    auto targetToEnemy = PlayerBattleManager.GetMethodByName("targetToEnemy");
    HOOK(targetToEnemy, my_targetToEnemy, o_targetToEnemy);

    LoadClass SkillMasterData("", "SkillMasterData");
    EqLimit = SkillMasterData.GetPropertyByName("EqLimit");
    auto CreateSkillData = SkillMasterData.GetMethodByName("CreateSkillData");
    HOOK(CreateSkillData, my_CreateSkillData, o_CreateSkillData);

    LoadClass PlayerAttackBase("", "PlayerAttackBase");
    auto checkMobReaction = PlayerAttackBase.GetMethodByName("checkMobReaction");
    HOOK(checkMobReaction, my_checkMobReaction, o_checkMobReaction);
    auto checkCriticalPercent = PlayerAttackBase.GetMethodByName("checkCriticalPercent");
    HOOK(checkCriticalPercent, my_checkCriticalPercent, o_checkCriticalPercent);
    auto checkAbnormalPercent = PlayerAttackBase.GetMethodByName("checkAbnormalPercent");
    HOOK(checkAbnormalPercent, my_checkAbnormalPercent, o_checkAbnormalPercent);
    auto ChackCorrectHit = PlayerAttackBase.GetMethodByName("ChackCorrectHit");
    HOOK(ChackCorrectHit, my_ChackCorrectHit, o_ChackCorrectHit);

    LoadClass ChatWindow("", "ChatWindow");
    auto OnSubmit = ChatWindow.GetMethodByName("OnSubmit", 0);
    HOOK(OnSubmit, my_OnSubmit, o_OnSubmit);

    AddNPCMessage = ChatWindow.GetMethodByName("AddNPCMessage", 2);

    inputLabel = ChatWindow.GetFieldByName("inputLabel");

    LoadClass UILabel("", "UILabel");
    mText = UILabel.GetFieldByName("mText");

    LoadClass SkillActionBase("", "SkillActionBase");
    auto checkPercent = SkillActionBase.GetMethodByName("checkPercent");
    HOOK(checkPercent, my_checkPercent, o_checkPercent);
    auto CalcDamage = SkillActionBase.GetMethodByName("CalcDamage", 2);
    HOOK(CalcDamage, my_CalcDamage, o_CalcDamage);
    auto CalcStablePercent = SkillActionBase.GetMethodByName("CalcStablePercent");
    HOOK(CalcStablePercent, my_CalcStablePercent, o_CalcStablePercent);
    auto CalcMagicStablePercent = SkillActionBase.GetMethodByName("CalcMagicStablePercent");
    HOOK(CalcMagicStablePercent, my_CalcMagicStablePercent, o_CalcMagicStablePercent);

    LoadClass PlayerDataManager("", "PlayerDataManager");
    FindPlayer = PlayerDataManager.GetMethodByName("FindPlayer", 0);

    LoadClass PlayerBattleStatus("", "PlayerBattleStatus");
    auto GetMotionSpeed = PlayerBattleStatus.GetMethodByName("GetMotionSpeed", 1);
    HOOK(GetMotionSpeed, my_GetMotionSpeed, o_GetMotionSpeed);

    auto GetNextAtkTime = PlayerBattleStatus.GetMethodByName("GetNextAtkTime", 1);
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
    shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false);

    LOGD("Thread started! Config file: %s", str.c_str());

    path = str;

    sleep(5);

    void *handle = BNM_dlopen("libil2cpp.so");

    LOGD("Il2cpp handle: %p", handle);

    BNM::External::TryLoad(handle);

    readConfig();

    il2cppHack();
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
    zygisk::Api *api;
    JNIEnv *env;
    std::string appDir;
};

REGISTER_ZYGISK_MODULE(ToramHack)