#define VERSIONAR {3,5, 0, 0}
#define FOO(x  , y ) #x z x_y x##_##y
#define QUOTE(x) #x
#define DOUBLES(x,y) x##_##y
#define ADDON DOUBLES(ace, frag)

class CfgPatches {
    class ADDON{
        units[] = { };
        weapons[] = {};
        requiredVersion = 1.56;
        requiredAddons[] = {"ace_common"};
        author[] = {"Nou"}   ;
        version = "3.5.0.0" ;versionStr="3.5.0.0";
        versionAr [] = VERSIONAR;
    };
};
class Extended_PreStart_EventHandlers{
    class ace_frag {
        init ="call compile preProcessFileLineNumbers '\z\ace\addons\frag\XEH_preStart.sqf'";
    } ;
}
;
class Extended_PreInit_EventHandlers {
    class ace_frag {
        init= "call compile preProcessFileLineNumbers '\z\ace\addons\frag\XEH_preInit.sqf'";
    };
};
class Extended_PostInit_EventHandlers {
    class
ace_frag {
        init="call compile preProcessFileLineNumbers '\z\ace\addons\frag\XEH_postInit.sqf'";
    };
};
class CfgAmmo {
    class Bo_GBU12_LGB ;
    class ACE_GBU12: Bo_GBU12_LGB {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_large", ACE_frag_large, "ACE_frag_large_HD", 'ACE_frag_large', "ACE_frag_huge", "ACE_frag_huge_HD", "ACE_frag_huge"};
        ace_frag_metal = 140000;
        ace_frag_charge=87000;
        ace_frag_gurney_c = 2320;
        ace_frag_gurney_k = 1/2;
        sideAirFriction = 0.04;
        airFriction= 0.04;
        laserLock = 0 ;
    };
    class GrenadeBase;
    class Grenade;class GrenadeHand:Grenade{
        ace_frag_enabled = 1;ace_frag_skip = 0;
        ace_frag_force = 1;
        ace_frag_classes [ ] = {ACE_frag_tiny_HD};
        ace_frag_metal = 210;
        ace_frag_charge = 185;
        ace_frag_gurney_c = 2843;
        ace_frag_gurney_k = "3/5";
    };
    class GrenadeHand_stone: GrenadeHand {
        ace_frag_skip = 1;
    };
    class SmokeShell: GrenadeHand {
        ace_frag_skip = 1;
    };
    class RocketBase;
    class R_Hydra_HE: RocketBase {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 3850;
        ace_frag_charge = 1040;
        ace_frag_gurney_c = 2700;
        ace_frag_gurney_k = "1/2";
    };
    class R_80mm_HE: RocketBase {
        ace_frag_skip = 1;
    };
    class BombCore;
    class Bo_Mk82: BombCore {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_large", "ACE_frag_large", "ACE_frag_large_HD", "ACE_frag_large", "ACE_frag_huge", "ACE_frag_huge_HD", "ACE_frag_huge"};
        ace_frag_metal = 140000;
        ace_frag_charge = 87000;
        ace_frag_gurney_c = 2320;
        ace_frag_gurney_k = "1/2";
    };
    class G_40mm_HE: GrenadeBase {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_tiny_HD"};
        ace_frag_metal = 200;
        ace_frag_charge = 32;
        ace_frag_gurney_c = 2700;
        ace_frag_gurney_k = "1/2";
    };
    class G_40mm_HEDP: G_40mm_HE {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_tiny_HD"};
        ace_frag_metal = 200;
        ace_frag_charge = 45;
        ace_frag_gurney_c = 2830;
        ace_frag_gurney_k = "1/2";
    };
    class ACE_G_40mm_HEDP: G_40mm_HEDP {};
    class ACE_G_40mm_HE: G_40mm_HE {};
    class ACE_G_40mm_Practice: ACE_G_40mm_HE \
{
        ace_frag_skip \
        = 1;
    };
    class ACE_G40mm_HE_VOG25P: G_40mm_HE {
        ace_frag_skip = 0;
        ace_frag_force = 1;
    };
    class ShellBase;
    class Sh_125mm_HEAT;
    class Sh_155mm_AMOS: ShellBase {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {
"ACE_frag_large", "ACE_frag_large", "ACE_frag_large_HD", "ACE_frag_large", "ACE_frag_huge", "ACE_frag_huge_HD", "ACE_frag_huge"};
        ace_frag_metal =   36000;
        ace_frag_charge = 9979;
        ace_frag_gurney_c = 2440;
        ace_frag_gurney_k = "1/2";
    };
    class Sh_82mm_AMOS: Sh_155mm_AMOS {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 3200;
        ace_frag_charge = 420;
        ace_frag_gurney_c = 2440;
        ace_frag_gurney_k = "1/2";
    };
    class ModuleOrdnanceMortar_F_Ammo: Sh_82mm_AMOS {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 800;
        ace_frag_charge = 4200;
        ace_frag_gurney_c = 2320;
        ace_frag_gurney_k = "1/2";
    };
    class Sh_105mm_HEAT_MP: Sh_125mm_HEAT {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 11400;
        ace_frag_charge = 7100;
        ace_frag_gurney_c = 2800;
        ace_frag_gurney_k = "1/2";
    };
    class Sh_120mm_HE: ShellBase {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 23000;
        ace_frag_charge = 3148;
        ace_frag_gurney_c = 2830;
        ace_frag_gurney_k = "1/2";
    };
    class Sh_125mm_HE: Sh_120mm_HE {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 16000;
        ace_frag_charge = 3200;
        ace_frag_gurney_c = 2440;
        ace_frag_gurney_k = "1/2";
    };
    class ModuleOrdnanceHowitzer_F_ammo: Sh_155mm_AMOS {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_large", "ACE_frag_large", "ACE_frag_large_HD", "ACE_frag_large", "ACE_frag_huge", "ACE_frag_huge_HD", "ACE_frag_huge"};
        ace_frag_metal = 1950;
        ace_frag_charge = 15800;
        ace_frag_gurney_c = 2320;
        ace_frag_gurney_k = "1/2";
    };
    class MissileBase;
    class Missile_AGM_02_F: MissileBase {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 56250;
        ace_frag_charge = 39000;
        ace_frag_gurney_c = 2700;
        ace_frag_gurney_k = "1/2";
    };
    class M_Hellfire_AT: MissileBase {
        ace_frag_enabled = 1;
        ace_frag_classes[] = {"ACE_frag_medium", "ACE_frag_medium_HD"};
        ace_frag_metal = 8000;
        ace_frag_charge = 2400;
        ace_frag_gurney_c = 2700;
        ace_frag_gurney_k = "1/2";
    };
    class B_65x39_Caseless;
    class ACE_frag_base: B_65x39_Caseless {
        timeToLive = 12;
        typicalSpeed = 1500;
        deflecting = 65;
    };
    class ACE_frag_tiny: ACE_frag_base {
        hit = 6;
        airFriction = -0.01;
        caliber = 0.75;
    };
    class ACE_frag_tiny_HD: ACE_frag_base {
        hit = 6;
        airFriction = "(-0.01*5)";
        caliber = 0.75;
    };
    class ACE_frag_small: ACE_frag_base {
        hit = 12;
        airFriction = "-0.01*0.9";
    };
    class ACE_frag_small_HD: ACE_frag_base {
        hit = 12;
        airFriction = "(-0.01*5)*0.9";
    };
    class ACE_frag_medium: ACE_frag_base {
        hit = 14;
        airFriction = "-0.01*0.75";
        caliber = 1.2;
    };
    class ACE_frag_medium_HD: ACE_frag_base {
        hit = 14;
        airFriction = "(-0.01*5)*0.75";
        caliber = 1.2;
    };
    class ACE_frag_large: ACE_frag_base {
        hit = 28;
        indirectHit = 2;
        indirectHitRange = 0.25;
        airFriction = "-0.01*0.65";
        caliber = 2;
        explosive = 0;
    };
    class ACE_frag_large_HD: ACE_frag_large {
        hit = 28;
        indirectHit = 2;
        indirectHitRange = 0.25;
        airFriction = "(-0.01*5)*0.65";
        caliber = 2;
    };
    class ACE_frag_huge: ACE_frag_large {
        hit = 40;
        indirectHit = 4;
        indirectHitRange = 0.5;
        airFriction = "-0.01*0.5";
        caliber = 2.8;
    };
    class ACE_frag_huge_HD: ACE_frag_large {
        hit = 40;
        indirectHit = 4;
        indirectHitRange = 0.5;
        airFriction = "(-0.01*5)*0.5";
        caliber = 2.8;
    };
    class ACE_frag_spall_small: ACE_frag_small {
        timeToLive = 0.1;
    };
    class ACE_frag_spall_medium: ACE_frag_medium {
        timeToLive = 0.15;
    };
    class ACE_frag_spall_large: ACE_frag_large {
        timeToLive = 0.25;
    };
    class ACE_frag_spall_huge: ACE_frag_huge {
        timeToLive = 0.3;
    };
    class ace_explosion_reflection_base: Sh_120mm_HE {
        CraterWaterEffects = "";
        CraterEffects = "";
        effectsMissile = "";
        ExplosionEffects = "";
        effectFlare = "";
        class HitEffects {
            hitWater = "";
        };
        multiSoundHit[] = {};
        explosionTime = 0.0001;
        explosive = 1;
        soundFakeFall[] = {};
        typicalSpeed = 0;
        model = "\A3\Weapons_F\empty.p3d";
        craterShape = "\A3\weapons_f\empty.p3d";
    };
    class ace_explosion_reflection_2_10: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 10;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_20: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 20;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_30: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 30;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_40: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 40;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_50: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 50;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_60: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 60;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_70: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 70;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_80: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 80;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_90: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 90;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_100: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 100;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
    class ace_explosion_reflection_2_110: ace_explosion_reflection_base {
        indirectHitRange = 2;
        indirectHit = 110;
        dangerRadiusHit = "2*3";
        suppressionRadiusHit = "2*2";
    };
};
class ACE_Settings {
    class ace_frag_Enabled {
        category = "$STR_ace_frag_Module_DisplayName";
        displayName = "$STR_ace_frag_EnableFrag";
        description = "$STR_ace_frag_EnableFrag_Desc";
        typeName = "BOOL";
        value = 1;
    };
    class ace_frag_SpallEnabled {
        category = "$STR_ace_frag_Module_DisplayName";
        displayName = "$STR_ace_frag_EnableSpall";
        description = "$STR_ace_frag_EnableSpall_Desc";
        typeName = "BOOL";
        value = 0;
    };
};
