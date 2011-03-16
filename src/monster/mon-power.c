/*
 * File: src/monster/mon-power.c
 * Purpose: functions for monster power evaluation
 *
 * Copyright (c) 2000-11 Chris Carr, Chris Robertson, Andrew Doull
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "monster/constants.h"
#include "monster/mon-power.h"

s32b tot_mon_power;

static long eval_blow_effect(int effect, int atk_dam, int rlev)
{
	switch (effect)
	{
		/*other bad effects - minor*/
		case RBE_EAT_GOLD:
		case RBE_EAT_ITEM:
		case RBE_EAT_FOOD:
		case RBE_EAT_LIGHT:
		case RBE_LOSE_CHR:
		{
			atk_dam += 5;
			break;
		}
		/*other bad effects - poison / disease */
		case RBE_POISON:
		{
			atk_dam *= 5;
			atk_dam /= 4;
			atk_dam += rlev;
			break;
		}
		/*other bad effects - elements / sustains*/
		case RBE_TERRIFY:
		case RBE_ELEC:
		case RBE_COLD:
		case RBE_FIRE:
		{
			atk_dam += 10;
			break;
		}
		/*other bad effects - elements / major*/
		case RBE_ACID:
		case RBE_BLIND:
		case RBE_CONFUSE:
		case RBE_LOSE_STR:
		case RBE_LOSE_INT:
		case RBE_LOSE_WIS:
		case RBE_LOSE_DEX:
		case RBE_HALLU:
		{
			atk_dam += 20;
			break;
		}
		/*other bad effects - major*/
		case RBE_UN_BONUS:
		case RBE_UN_POWER:
		case RBE_LOSE_CON:
		{
			atk_dam += 30;
			break;
		}
		/*other bad effects - major*/
		case RBE_PARALYZE:
		case RBE_LOSE_ALL:
		{
			atk_dam += 40;
			break;
		}
		/* Experience draining attacks */
		case RBE_EXP_10:
		case RBE_EXP_20:
		{
			/* change inspired by Eddie because exp is infinite */
			atk_dam += 5;
			break;
		}
		case RBE_EXP_40:
		case RBE_EXP_80:
		{
			/* as above */
			atk_dam += 10;
			break;
		}
		/*Earthquakes*/
		case RBE_SHATTER:
		{
			atk_dam += 300;
			break;
		}
		/*nothing special*/
		default: break;
	}

	return (atk_dam);
}

static long eval_max_dam(monster_race *r_ptr)
{
	int hp, rlev, i;
	int melee_dam, atk_dam, spell_dam, breath_dam;
	int dam = 1;

	/*clear the counters*/
	melee_dam = breath_dam = atk_dam = spell_dam = 0;

	/* Evaluate average HP for this monster */
	hp = r_ptr->avg_hp;

	/* Extract the monster level, force 1 for town monsters */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Assume single resist for the elemental attacks */
	if (rsf_has(r_ptr->spell_flags, RSF_BR_ACID))
	{
		breath_dam = (RES_ACID_ADJ(MIN(BR_ACID_MAX,
			(hp / BR_ACID_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 20;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_ELEC))
	{
		breath_dam = (RES_ELEC_ADJ(MIN(BR_ELEC_MAX, 
			(hp / BR_ELEC_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 10;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_FIRE))
	{
		breath_dam = (RES_FIRE_ADJ(MIN(BR_FIRE_MAX,
			(hp / BR_FIRE_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 10;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_COLD))
	{
		breath_dam = (RES_COLD_ADJ(MIN(BR_COLD_MAX,
			(hp / BR_COLD_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 10;
	}
	/* Same for poison, but lower damage cap */
	if (rsf_has(r_ptr->spell_flags, RSF_BR_POIS))
	{
		breath_dam = (RES_POIS_ADJ(MIN(BR_POIS_MAX,
			(hp / BR_POIS_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = (breath_dam * 5 / 4)
			+ rlev;
	}
	/*
	 * Same formula for the high resist attacks
	 * (remember, we are assuming maximum resisted damage
	 * so we *minimise* the resistance)
	 * See also: melee2.c, spells1.c, constants.h
	 */
	if (rsf_has(r_ptr->spell_flags, RSF_BR_NETH))
	{
		breath_dam = (RES_NETH_ADJ(MIN(BR_NETH_MAX,
			(hp / BR_NETH_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 2000
			/ (rlev + 1);
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_CHAO))
	{
		breath_dam = (RES_CHAO_ADJ(MIN(BR_CHAO_MAX,
			(hp / BR_CHAO_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 2000
			/ (rlev + 1);
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_DISE))
	{
		breath_dam = (RES_DISE_ADJ(MIN(BR_DISE_MAX,
			(hp / BR_DISE_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 50;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_SHAR))
	{
		breath_dam = (RES_SHAR_ADJ(MIN(BR_SHAR_MAX,
			(hp / BR_SHAR_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = (breath_dam * 5 / 4)
			+ 5;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_LIGHT))
	{
		breath_dam = (RES_LIGHT_ADJ(MIN(BR_LIGHT_MAX,
			(hp / BR_LIGHT_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 10;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_DARK))
	{
		breath_dam = (RES_DARK_ADJ(MIN(BR_DARK_MAX,
			(hp / BR_DARK_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 10;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_SOUN))
	{
		breath_dam = (RES_SOUN_ADJ(MIN(BR_SOUN_MAX,
			(hp / BR_SOUN_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 20;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_NEXU))
	{
		breath_dam = (RES_NEXU_ADJ(MIN(BR_NEXU_MAX,
			(hp / BR_NEXU_DIVISOR)), MINIMISE));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 20;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_TIME))
	{
		breath_dam = MIN(BR_TIME_MAX, (hp / BR_TIME_DIVISOR));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 2000 / (rlev + 1);
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_INER))
	{
		breath_dam = MIN(BR_INER_MAX, (hp / BR_INER_DIVISOR));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 30;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_GRAV))
	{
		breath_dam = MIN(BR_GRAV_MAX, (hp / BR_GRAV_DIVISOR));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 30;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_PLAS))
	{
		breath_dam = MIN(BR_PLAS_MAX, (hp / BR_PLAS_DIVISOR));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 30;
	}
	if (rsf_has(r_ptr->spell_flags, RSF_BR_WALL))
	{
		breath_dam = MIN(BR_FORC_MAX, (hp / BR_FORC_DIVISOR));
		if (spell_dam < breath_dam) spell_dam = breath_dam + 30;
	}
	/* Handle the attack spells, again assuming minimised single resists for max damage */
	if (rsf_has(r_ptr->spell_flags, RSF_BA_ACID) && spell_dam < (RES_ACID_ADJ(BA_ACID_DMG(rlev, MAXIMISE), MINIMISE) + 20))
		spell_dam = (RES_ACID_ADJ(BA_ACID_DMG(rlev, MAXIMISE), MINIMISE) + 20);
	if (rsf_has(r_ptr->spell_flags, RSF_BA_ELEC) && spell_dam < (RES_ELEC_ADJ(BA_ELEC_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_ELEC_ADJ(BA_ELEC_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	if (rsf_has(r_ptr->spell_flags, RSF_BA_FIRE) && spell_dam < (RES_FIRE_ADJ(BA_FIRE_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_FIRE_ADJ(BA_FIRE_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	if (rsf_has(r_ptr->spell_flags, RSF_BA_COLD) && spell_dam < (RES_COLD_ADJ(BA_COLD_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_COLD_ADJ(BA_COLD_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	if (rsf_has(r_ptr->spell_flags, RSF_BA_POIS) && spell_dam < 8)
		spell_dam = 8;
	if (rsf_has(r_ptr->spell_flags, RSF_BA_NETH) && spell_dam < (RES_NETH_ADJ(BA_NETH_DMG(rlev, MAXIMISE), MINIMISE) + 2000 / (rlev + 1)))
		spell_dam = (RES_NETH_ADJ(BA_NETH_DMG(rlev, MAXIMISE), MINIMISE) + 2000 / (rlev + 1));
	if (rsf_has(r_ptr->spell_flags, RSF_BA_WATE) && spell_dam < (BA_WATE_DMG(rlev, MAXIMISE) + 20))
		spell_dam = (BA_WATE_DMG(rlev, MAXIMISE) + 20);
	if (rsf_has(r_ptr->spell_flags, RSF_BA_MANA) && spell_dam < (BA_MANA_DMG(rlev, MAXIMISE) + 100))
		spell_dam = (BA_MANA_DMG(rlev, MAXIMISE) + 100);
	if (rsf_has(r_ptr->spell_flags, RSF_BA_DARK) && spell_dam < (RES_DARK_ADJ(BA_DARK_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_DARK_ADJ(BA_DARK_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	/* Small annoyance value */
	if (rsf_has(r_ptr->spell_flags, RSF_DRAIN_MANA) && spell_dam < 5)
		spell_dam = 5;
	/* For all attack forms the player can save against, spell_damage is halved */
	if (rsf_has(r_ptr->spell_flags, RSF_MIND_BLAST) && spell_dam < (MIND_BLAST_DMG(rlev, MAXIMISE) / 2))
		spell_dam = (MIND_BLAST_DMG(rlev, MAXIMISE) / 2);
	if (rsf_has(r_ptr->spell_flags, RSF_BRAIN_SMASH) && spell_dam < (BRAIN_SMASH_DMG(rlev, MAXIMISE) / 2))
		spell_dam = (BRAIN_SMASH_DMG(rlev, MAXIMISE) / 2);
	if (rsf_has(r_ptr->spell_flags, RSF_CAUSE_1) && spell_dam < (CAUSE1_DMG(rlev, MAXIMISE) / 2))
		spell_dam = (CAUSE1_DMG(rlev, MAXIMISE) / 2);
	if (rsf_has(r_ptr->spell_flags, RSF_CAUSE_2) && spell_dam < (CAUSE2_DMG(rlev, MAXIMISE) / 2))
		spell_dam = (CAUSE2_DMG(rlev, MAXIMISE) / 2);
	if (rsf_has(r_ptr->spell_flags, RSF_CAUSE_3) && spell_dam < (CAUSE3_DMG(rlev, MAXIMISE) / 2))
		spell_dam = (CAUSE3_DMG(rlev, MAXIMISE) / 2);
	if (rsf_has(r_ptr->spell_flags, RSF_CAUSE_4) && spell_dam < (CAUSE4_DMG(rlev, MAXIMISE) / 2))
		spell_dam = (CAUSE4_DMG(rlev, MAXIMISE) / 2);
	if (rsf_has(r_ptr->spell_flags, RSF_BO_ACID) && spell_dam < (RES_ACID_ADJ(BO_ACID_DMG(rlev, MAXIMISE), MINIMISE) + 20))
		spell_dam = (RES_ACID_ADJ(BO_ACID_DMG(rlev, MAXIMISE), MINIMISE) + 20);
	if (rsf_has(r_ptr->spell_flags, RSF_BO_ELEC) && spell_dam < (RES_ELEC_ADJ(BO_ELEC_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_ELEC_ADJ(BO_ELEC_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	if (rsf_has(r_ptr->spell_flags, RSF_BO_FIRE) && spell_dam < (RES_FIRE_ADJ(BO_FIRE_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_FIRE_ADJ(BO_FIRE_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	if (rsf_has(r_ptr->spell_flags, RSF_BO_COLD) && spell_dam < (RES_COLD_ADJ(BO_COLD_DMG(rlev, MAXIMISE), MINIMISE) + 10))
		spell_dam = (RES_COLD_ADJ(BO_COLD_DMG(rlev, MAXIMISE), MINIMISE) + 10);
	if (rsf_has(r_ptr->spell_flags, RSF_BO_NETH) && spell_dam < (RES_NETH_ADJ(BO_NETH_DMG(rlev, MAXIMISE), MINIMISE) + 2000 / (rlev + 1)))
		spell_dam = (RES_NETH_ADJ(BO_NETH_DMG(rlev, MAXIMISE), MINIMISE) + 2000 / (rlev + 1));
	if (rsf_has(r_ptr->spell_flags, RSF_BO_WATE) && spell_dam < (BO_WATE_DMG(rlev, MAXIMISE) + 20))
		spell_dam = (BO_WATE_DMG(rlev, MAXIMISE) + 20);
	if (rsf_has(r_ptr->spell_flags, RSF_BO_MANA) && spell_dam < (BO_MANA_DMG(rlev, MAXIMISE)))
		spell_dam = (BO_MANA_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_BO_PLAS) && spell_dam < (BO_PLAS_DMG(rlev, MAXIMISE)))
		spell_dam = (BO_PLAS_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_BO_ICEE) && spell_dam < (RES_COLD_ADJ(BO_ICEE_DMG(rlev, MAXIMISE), MINIMISE)))
		spell_dam = (RES_COLD_ADJ(BO_ICEE_DMG(rlev, MAXIMISE), MINIMISE));
	/* Projectile spells */
	if (rsf_has(r_ptr->spell_flags, RSF_ARROW_1) && spell_dam < (ARROW1_DMG(rlev, MAXIMISE)))
		spell_dam = (ARROW1_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_ARROW_2) && spell_dam < (ARROW2_DMG(rlev, MAXIMISE)))
		spell_dam = (ARROW2_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_ARROW_3) && spell_dam < (ARROW3_DMG(rlev, MAXIMISE)))
		spell_dam = (ARROW3_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_ARROW_4) && spell_dam < (ARROW4_DMG(rlev, MAXIMISE)))
		spell_dam = (ARROW4_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_BOULDER) && spell_dam < (BOULDER_DMG(rlev, MAXIMISE)))
		spell_dam = (BOULDER_DMG(rlev, MAXIMISE));
	if (rsf_has(r_ptr->spell_flags, RSF_MISSILE) && spell_dam < (MISSILE_DMG(rlev, MAXIMISE)))
		spell_dam = (MISSILE_DMG(rlev, MAXIMISE));
	/* Small annoyance value */
	if (rsf_has(r_ptr->spell_flags, RSF_SCARE) && spell_dam < 5)
		spell_dam = 5;
	/* Somewhat higher annoyance values */
	if (rsf_has(r_ptr->spell_flags, RSF_BLIND) && spell_dam < 10)
		spell_dam = 10;
	if (rsf_has(r_ptr->spell_flags, RSF_CONF) && spell_dam < 10)
		spell_dam = 10;
	/* A little more dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_SLOW) && spell_dam < 15)
		spell_dam = 15;
	/* Quite dangerous at an early level */
	if (rsf_has(r_ptr->spell_flags, RSF_HOLD) && spell_dam < 25)
		spell_dam = 25;
	/* Arbitrary values along similar lines from here on */
	if (rsf_has(r_ptr->spell_flags, RSF_HASTE) && spell_dam < 70)
		spell_dam = 70;
	if (rsf_has(r_ptr->spell_flags, RSF_HEAL) && spell_dam < 30)
		spell_dam = 30;
	if (rsf_has(r_ptr->spell_flags, RSF_BLINK) && spell_dam < 15)
		spell_dam = 15;
	if (rsf_has(r_ptr->spell_flags, RSF_TELE_TO) && spell_dam < 25)
		spell_dam = 25;
	if (rsf_has(r_ptr->spell_flags, RSF_TELE_AWAY) && spell_dam < 25)
		spell_dam = 25;
	if (rsf_has(r_ptr->spell_flags, RSF_TELE_LEVEL) && spell_dam < 40)
		spell_dam = 40;
	if (rsf_has(r_ptr->spell_flags, RSF_DARKNESS) && spell_dam < 5)
		spell_dam = 5;
	if (rsf_has(r_ptr->spell_flags, RSF_TRAPS) && spell_dam < 10)
		spell_dam = 10;
	if (rsf_has(r_ptr->spell_flags, RSF_FORGET) && spell_dam < 25)
		spell_dam = 25;
	/* All summons are assigned arbitrary values */
	/* Summon kin is more dangerous at deeper levels */
	if (rsf_has(r_ptr->spell_flags, RSF_S_KIN) && spell_dam < rlev * 2)
		spell_dam = rlev * 2;
	/* Shriek can bring dangerous combinations together */
	if (rsf_has(r_ptr->spell_flags, RSF_SHRIEK) && spell_dam < rlev * 3 / 2)
		spell_dam = rlev * 3 / 2;
	/* Dangerous! */
	if (rsf_has(r_ptr->spell_flags, RSF_S_HI_DEMON) && spell_dam < 250)
		spell_dam = 250;
	/* Somewhat dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_MONSTER) && spell_dam < 40)
		spell_dam = 40;
	/* More dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_MONSTERS) && spell_dam < 80)
		spell_dam = 80;
	/* Mostly just annoying */
	if (rsf_has(r_ptr->spell_flags, RSF_S_ANIMAL) && spell_dam < 30)
		spell_dam = 30;
	if (rsf_has(r_ptr->spell_flags, RSF_S_SPIDER) && spell_dam < 20)
		spell_dam = 20;
	/* Can be quite dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_HOUND) && spell_dam < 100)
		spell_dam = 100;
	/* Dangerous! */
	if (rsf_has(r_ptr->spell_flags, RSF_S_HYDRA) && spell_dam < 150)
		spell_dam = 150;
	/* Can be quite dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_ANGEL) && spell_dam < 150)
		spell_dam = 150;
	/* All of these more dangerous at higher levels */
	if (rsf_has(r_ptr->spell_flags, RSF_S_DEMON) && spell_dam < (rlev * 3) / 2)
		spell_dam = (rlev * 3) / 2;
	if (rsf_has(r_ptr->spell_flags, RSF_S_UNDEAD) && spell_dam < (rlev * 3) / 2)
		spell_dam = (rlev * 3) / 2;
	if (rsf_has(r_ptr->spell_flags, RSF_S_DRAGON) && spell_dam < (rlev * 3) / 2)
		spell_dam = (rlev * 3) / 2;
	/* Extremely dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_HI_UNDEAD) && spell_dam < 400)
		spell_dam = 400;
	/* Extremely dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_HI_DRAGON) && spell_dam < 400)
		spell_dam = 400;
	/* Extremely dangerous */
	if (rsf_has(r_ptr->spell_flags, RSF_S_WRAITH) && spell_dam < 450)
		spell_dam = 450;
	/* Most dangerous summon */
	if (rsf_has(r_ptr->spell_flags, RSF_S_UNIQUE) && spell_dam < 500)
		spell_dam = 500;

	/* Hack - Apply over 10 rounds */
	spell_dam *= 10;

	/* Scale for frequency and availability of mana / ammo */
	if (spell_dam)
	{
		int freq = r_ptr->freq_spell;

			/* Hack -- always get 1 shot */
			if (freq < 10) freq = 10;

			/* Adjust for frequency */
			spell_dam = spell_dam * freq / 100;
	}

	/* Check attacks */
	for (i = 0; i < 4; i++)
	{
		/* Extract the attack infomation */
		int effect = r_ptr->blow[i].effect;
		int method = r_ptr->blow[i].method;
		int d_dice = r_ptr->blow[i].d_dice;
		int d_side = r_ptr->blow[i].d_side;

		/* Hack -- no more attacks */
		if (!method) continue;

		/* Assume maximum damage*/
		atk_dam = eval_blow_effect(effect, d_dice * d_side, r_ptr->level);

		switch (method)
		{
				/*stun definitely most dangerous*/
				case RBM_PUNCH:
				case RBM_KICK:
				case RBM_BUTT:
				case RBM_CRUSH:
				{
					atk_dam *= 4;
					atk_dam /= 3;
					break;
				}
				/*cut*/
				case RBM_CLAW:
				case RBM_BITE:
				{
					atk_dam *= 7;
					atk_dam /= 5;
					break;
				}
				default: 
				{
					break;
				}
			}

			/* Normal melee attack */
			if (!rf_has(r_ptr->flags, RF_NEVER_BLOW))
			{
				/* Keep a running total */
				melee_dam += atk_dam;
			}
	}

		/* 
		 * Apply damage over 10 rounds. We assume that the monster has to make contact first.
		 * Hack - speed has more impact on melee as has to stay in contact with player.
		 * Hack - this is except for pass wall and kill wall monsters which can always get to the player.
		 * Hack - use different values for huge monsters as they strike out to range 2.
		 */
		if (flags_test(r_ptr->flags, RF_SIZE, RF_KILL_WALL, RF_PASS_WALL, FLAG_END))
				melee_dam *= 10;
		else
		{
			melee_dam = melee_dam * 3 + melee_dam * extract_energy[r_ptr->speed + (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)] / 7;
		}

		/*
		 * Scale based on attack accuracy. We make a massive number of assumptions here and just use monster level.
		 */
		melee_dam = melee_dam * MIN(45 + rlev * 3, 95) / 100;

		/* Hack -- Monsters that multiply ignore the following reductions */
		if (!rf_has(r_ptr->flags, RF_MULTIPLY))
		{
			/*Reduce damamge potential for monsters that move randomly */
			if (flags_test(r_ptr->flags, RF_SIZE, RF_RAND_25, RF_RAND_50, FLAG_END))
			{
				int reduce = 100;

				if (rf_has(r_ptr->flags, RF_RAND_25)) reduce -= 25;
				if (rf_has(r_ptr->flags, RF_RAND_50)) reduce -= 50;

				/*even moving randomly one in 8 times will hit the player*/
				reduce += (100 - reduce) / 8;

				/* adjust the melee damage*/
				melee_dam = (melee_dam * reduce) / 100;
			}

			/*monsters who can't move aren't nearly as much of a combat threat*/
			if (rf_has(r_ptr->flags, RF_NEVER_MOVE))
			{
				if (rsf_has(r_ptr->spell_flags, RSF_TELE_TO) ||
				    rsf_has(r_ptr->spell_flags, RSF_BLINK))
				{
					/* Scale for frequency */
					melee_dam = melee_dam / 5 + 4 * melee_dam * r_ptr->freq_spell / 500;

					/* Incorporate spell failure chance */
					if (!rf_has(r_ptr->flags, RF_STUPID)) melee_dam = melee_dam / 5 + 4 * melee_dam * MIN(75 + (rlev + 3) / 4, 100) / 500;
				}
				else if (rf_has(r_ptr->flags, RF_INVISIBLE)) melee_dam /= 3;
				else melee_dam /= 5;
			}
		}

		/* But keep at a minimum */
		if (melee_dam < 1) melee_dam = 1;

	/*
	 * Combine spell and melee damage
	 */
	dam = (spell_dam + melee_dam);
	
	r_ptr->highest_threat = dam;
	r_ptr->spell_dam = spell_dam;	/*AMF:DEBUG*/
	r_ptr->melee_dam = melee_dam;	/*AMF:DEBUG*/

	/*
	 * Adjust for speed.  Monster at speed 120 will do double damage,
	 * monster at speed 100 will do half, etc.  Bonus for monsters who can haste self.
	 */
	dam = (dam * extract_energy[r_ptr->speed + (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)]) / 10;

	/*
	 * Adjust threat for speed -- multipliers are more threatening.
	 */
	if (rf_has(r_ptr->flags, RF_MULTIPLY))
		r_ptr->highest_threat = (r_ptr->highest_threat * extract_energy[r_ptr->speed + (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)]) / 5;

	/*
	 * Adjust threat for friends.
	 */
	if (rf_has(r_ptr->flags, RF_FRIENDS))
		r_ptr->highest_threat *= 2;
	else if (rf_has(r_ptr->flags, RF_FRIEND))
		r_ptr->highest_threat = r_ptr->highest_threat * 3 / 2;
		
	/*but deep in a minimum*/
	if (dam < 1) dam  = 1;

	/* We're done */
	return (dam);
}

static long eval_hp_adjust(monster_race *r_ptr)
{
	long hp;
	int resists = 1;
	int hide_bonus = 0;

	/* Get the monster base hitpoints */
	hp = r_ptr->avg_hp;

	/* Never moves with no ranged attacks - high hit points count for less */
	if (rf_has(r_ptr->flags, RF_NEVER_MOVE) && !(r_ptr->freq_innate || r_ptr->freq_spell))
	{
		hp /= 2;
		if (hp < 1) hp = 1;
	}

	/* Just assume healers have more staying power */
	if (rsf_has(r_ptr->spell_flags, RSF_HEAL)) hp = (hp * 6) / 5;

	/* Miscellaneous improvements */
	if (rf_has(r_ptr->flags, RF_REGENERATE)) {hp *= 10; hp /= 9;}
	if (rf_has(r_ptr->flags, RF_PASS_WALL)) {hp *= 3; hp /= 2;}

	/* Calculate hide bonus */
	if (rf_has(r_ptr->flags, RF_EMPTY_MIND)) hide_bonus += 2;
	else
	{
		if (rf_has(r_ptr->flags, RF_COLD_BLOOD)) hide_bonus += 1;
		if (rf_has(r_ptr->flags, RF_WEIRD_MIND)) hide_bonus += 1;
	}

	/* Invisibility */
	if (rf_has(r_ptr->flags, RF_INVISIBLE))
	{
		hp = (hp * (r_ptr->level + hide_bonus + 1)) / MAX(1, r_ptr->level);
	}

	/* Monsters that can teleport are a hassle, and can easily run away */
	if (flags_test(r_ptr->spell_flags, RSF_SIZE, RSF_TPORT, RSF_TELE_AWAY,
	    RSF_TELE_LEVEL, FLAG_END))
		hp = (hp * 6) / 5;

	/*
 	 * Monsters that multiply are tougher to kill
	 */
	if (rf_has(r_ptr->flags, RF_MULTIPLY)) hp *= 2;

	/* Monsters with resistances are harder to kill.
	   Therefore effective slays / brands against them are worth more. */
	if (rf_has(r_ptr->flags, RF_IM_ACID)) resists += 2;
	if (rf_has(r_ptr->flags, RF_IM_FIRE)) resists += 2;
	if (rf_has(r_ptr->flags, RF_IM_COLD)) resists += 2;
	if (rf_has(r_ptr->flags, RF_IM_ELEC)) resists += 2;
	if (rf_has(r_ptr->flags, RF_IM_POIS)) resists += 2;

	/* Bonus for multiple basic resists and weapon resists */
	if (resists >= 12) resists *= 6;
	else if (resists >= 10) resists *= 4;
	else if (resists >= 8) resists *= 3;
	else if (resists >= 6) resists *= 2;

	/* If quite resistant, reduce resists by defense holes */
	if (resists >= 6)
	{
		if (rf_has(r_ptr->flags, RF_HURT_ROCK)) resists -= 1;
		if (rf_has(r_ptr->flags, RF_HURT_LIGHT)) resists -= 1;
		if (!rf_has(r_ptr->flags, RF_NO_SLEEP)) resists -= 3;
		if (!rf_has(r_ptr->flags, RF_NO_FEAR)) resists -= 2;
		if (!rf_has(r_ptr->flags, RF_NO_CONF)) resists -= 2;
		if (!rf_has(r_ptr->flags, RF_NO_STUN)) resists -= 1;

		if (resists < 5) resists = 5;
	}

	/* If quite resistant, bonus for high resists */
	if (resists >= 3)
	{
		if (rf_has(r_ptr->flags, RF_IM_WATER)) resists += 1;
		if (rf_has(r_ptr->flags, RF_RES_NETH)) resists += 1;
		if (rf_has(r_ptr->flags, RF_RES_NEXUS)) resists += 1;
		if (rf_has(r_ptr->flags, RF_RES_DISE)) resists += 1;
	}

	/* Scale resists */
	resists = resists * 25;

	/* Monster resistances */
	if (resists < (r_ptr->ac + resists) / 3)
	{
		hp += (hp * resists) / (150 + r_ptr->level); 	
	}
	else
	{
		hp += (hp * (r_ptr->ac + resists) / 3) / (150 + r_ptr->level); 			
	}

	/*boundry control*/
	if (hp < 1) hp = 1;

	return (hp);

}

errr eval_r_power(struct monster_race *races)
{
	int i, j, iteration;
	byte lvl;
	long hp, av_hp, dam, av_dam, *power;
	long tot_hp[MAX_DEPTH], tot_dam[MAX_DEPTH], mon_count[MAX_DEPTH];
	monster_race *r_ptr = NULL;
	ang_file *mon_fp;
	char buf[1024];
	bool dump = FALSE;

	/* Allocate space for power */
	power = C_ZNEW(z_info->r_max, long);

for (iteration = 0; iteration < 3; iteration ++) {

	/* Reset the sum of all monster power values */
	tot_mon_power = 0;

	/* Make sure all arrays start at zero */
	for (i = 0; i < MAX_DEPTH; i++)	{
		tot_hp[i] = 0;
		tot_dam[i] = 0;
		mon_count[i] = 0;
	}

	/* Go through r_info and evaluate power ratings & flows. */
	for (i = 0; i < z_info->r_max; i++)	{

		/* Point at the "info" */
		r_ptr = &races[i];

		/* Set the current level */
		lvl = r_ptr->level;

		/* Maximum damage this monster can do in 10 game turns */
		dam = eval_max_dam(r_ptr);

		/* Adjust hit points based on resistances */
		hp = eval_hp_adjust(r_ptr);

		/* Hack -- set exp */
		if (lvl == 0)
			r_ptr->mexp = 0L;
		else {
			/* Compute depths of non-unique monsters */
			if (!rf_has(r_ptr->flags, RF_UNIQUE)) {
				long mexp = (hp * dam) / 25;
				long threat = r_ptr->highest_threat;

				/* Compute level algorithmically */
				for (j = 1; (mexp > j + 4) || (threat > j + 5);
					mexp -= j * j, threat -= (j + 4), j++);

				/* Set level */
				lvl = MIN(( j > 250 ? 90 + (j - 250) / 20 : /* Level 90+ */
						(j > 130 ? 70 + (j - 130) / 6 :	/* Level 70+ */
						(j > 40 ? 40 + (j - 40) / 3 :	/* Level 40+ */
						j))), 99);

				/* Set level */
				if (arg_rebalance)
					r_ptr->level = lvl;
			}

			if (arg_rebalance) {
				/* Hack -- for Ungoliant */
				if (hp > 10000)
					r_ptr->mexp = (hp / 25) * (dam / lvl);
				else r_ptr->mexp = (hp * dam) / (lvl * 25);

				/* Round to 2 significant figures */
				if (r_ptr->mexp > 100) {
					if (r_ptr->mexp < 1000) {
						r_ptr->mexp = (r_ptr->mexp + 5) / 10;
						r_ptr->mexp *= 10;
					}
					else if (r_ptr->mexp < 10000) {
						r_ptr->mexp = (r_ptr->mexp + 50) / 100;
						r_ptr->mexp *= 100;
					}
					else if (r_ptr->mexp < 100000) {
						r_ptr->mexp = (r_ptr->mexp + 500) / 1000;
						r_ptr->mexp *= 1000;
					}
					else if (r_ptr->mexp < 1000000) {
						r_ptr->mexp = (r_ptr->mexp + 5000) / 10000;
						r_ptr->mexp *= 10000;
					}
					else if (r_ptr->mexp < 10000000) {
						r_ptr->mexp = (r_ptr->mexp + 50000) / 100000;
						r_ptr->mexp *= 100000;
					}
				}
			}
		}

		/* If we're rebalancing, this is a nop, if not, we restore the orig value */
		lvl = r_ptr->level;
		if ((lvl) && (r_ptr->mexp < 1L))
			r_ptr->mexp = 1L;

		/*
		 * Hack - We have to use an adjustment factor to prevent overflow.
		 * Try to scale evenly across all levels instead of scaling by level.
		 */
		hp /= 2;
		if(hp < 1)
			hp = 1;
		r_ptr->hp = hp;		/*AMF:DEBUG*/

		/* Define the power rating */
		power[i] = hp * dam;

		/* Adjust for group monsters.  Average in-level group size is 5 */
		if (!rf_has(r_ptr->flags, RF_UNIQUE)) {
			if (rf_has(r_ptr->flags, RF_FRIEND))
				power[i] *= 2;
			else if (rf_has(r_ptr->flags, RF_FRIENDS))
				power[i] *= 5;
		}
	
		/* Adjust for escorts */
		if (rf_has(r_ptr->flags, RF_ESCORTS))
			power[i] *= 3;
		if (rf_has(r_ptr->flags, RF_ESCORT) && !rf_has(r_ptr->flags, RF_ESCORTS))
			power[i] *= 2;

		/* Adjust for multiplying monsters. This is modified by the speed,
		 * as fast multipliers are much worse than slow ones. We also adjust for
		 * ability to bypass walls or doors.
		 */
		if (rf_has(r_ptr->flags, RF_MULTIPLY)) {
			if (flags_test(r_ptr->flags, RF_SIZE, RF_KILL_WALL, RF_PASS_WALL, FLAG_END))
				power[i] = MAX(power[i], power[i] * extract_energy[r_ptr->speed
					+ (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)]);
			else if (flags_test(r_ptr->flags, RF_SIZE, RF_OPEN_DOOR, RF_BASH_DOOR, FLAG_END))
				power[i] = MAX(power[i], power[i] *  extract_energy[r_ptr->speed
					+ (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)] * 3 / 2);
			else
				power[i] = MAX(power[i], power[i] * extract_energy[r_ptr->speed
					+ (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)] / 2);
		}

		/*
		 * Update the running totals - these will be used as divisors later
		 * Total HP / dam / count for everything up to the current level
		 */
		for (j = lvl; j < (lvl == 0 ? lvl + 1: MAX_DEPTH); j++)	{
			int count = 10;

			/* Uniques don't count towards monster power on the level. */
			if (rf_has(r_ptr->flags, RF_UNIQUE)) continue;

			/* Specifically placed monsters don't count towards monster power
			 * on the level. */
			if (!(r_ptr->rarity)) continue;

			/* Hack -- provide adjustment factor to prevent overflow */
			if ((j == 90) && (r_ptr->level < 90)) {
				hp /= 10;
				dam /= 10;
			}

			if ((j == 65) && (r_ptr->level < 65)) {
				hp /= 10;
				dam /= 10;
			}

			if ((j == 40) && (r_ptr->level < 40)) {
				hp /= 10;
				dam /= 10;
			}

			/*
			 * Hack - if it's a group monster or multiplying monster, add several to the count
			 * so that the averages don't get thrown off
			 */

			if (rf_has(r_ptr->flags, RF_FRIEND))
				count = 20;
			else if (rf_has(r_ptr->flags, RF_FRIENDS))
				count = 50;

			if (rf_has(r_ptr->flags, RF_MULTIPLY)) {
				if (flags_test(r_ptr->flags, RF_SIZE, RF_KILL_WALL, RF_PASS_WALL, FLAG_END))
					count = MAX(1, extract_energy[r_ptr->speed
						+ (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)]) * count;
				else if (flags_test(r_ptr->flags, RF_SIZE, RF_OPEN_DOOR, RF_BASH_DOOR, FLAG_END))
					count = MAX(1, extract_energy[r_ptr->speed
						+ (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)] * 3 / 2) * count;
				else
					count = MAX(1, extract_energy[r_ptr->speed
						+ (rsf_has(r_ptr->spell_flags, RSF_HASTE) ? 5 : 0)] / 2) * count;
			}

			/* Very rare monsters count less towards total monster power on the
			 * level. */
			if (r_ptr->rarity > count) {
				hp = hp * count / r_ptr->rarity;
				dam = dam * count / r_ptr->rarity;

				count = r_ptr->rarity;
			}

			tot_hp[j] += hp;
			tot_dam[j] += dam;

			mon_count[j] += count / r_ptr->rarity;
		}

	}

	/* Apply divisors now */
	for (i = 0; i < z_info->r_max; i++) {
		int new_power;

		/* Point at the "info" */
		r_ptr = &races[i];

		/* Extract level */
		lvl = r_ptr->level;

		/* Paranoia */
		if (tot_hp[lvl] != 0 && tot_dam[lvl] != 0) {

			/* Divide by average HP and av damage for all in-level monsters */
			/* Note we have factored in the above 'adjustment factor' */
			av_hp = tot_hp[lvl] * 10 / mon_count[lvl];
			av_dam = tot_dam[lvl] * 10 / mon_count[lvl];

			/* Assign monster power */
			r_ptr->power = power[i];

			/* Justifiable paranoia - avoid divide by zero errors */
			if (av_hp > 0)
				power[i] = power[i] / av_hp;
			if (av_dam > 0)
				power[i] = power[i] / av_dam;

			/* Assign monster scaled power */
			r_ptr->scaled_power = power[i];

			/* Never less than 1 */
			if (r_ptr->power < 1)
				r_ptr->power = 1;

			/* Get power */
			new_power = r_ptr->power;

			/* Compute rarity algorithmically */
			for (j = 1; new_power > j; new_power -= j * j, j++);

			/* Set rarity */
			if (arg_rebalance)
				r_ptr->rarity = j;
		}
	}

}
	/* Determine total monster power */
	for (i = 0; i < z_info->r_max; i++)
		tot_mon_power += r_info[i].scaled_power;

/*	msg("Tot mon power is %d", tot_mon_power); */

	if (dump) {
		/* dump the power details */
		path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "mon_power.txt");
		mon_fp = file_open(buf, MODE_WRITE, FTYPE_TEXT);

		file_putf(mon_fp, "ridx|level|rarity|d_char|name|pwr|scaled|melee|spell|hp\n");

		for (i = 0; i < z_info->r_max; i++) {
			r_ptr = &r_info[i];	

			/* Don't print anything for nonexistent monsters */
			if (!r_ptr->name) continue;

			file_putf(mon_fp, "%d|%d|%d|%c|%s|%d|%d|%d|%d|%d\n", r_ptr->ridx,
				r_ptr->level, r_ptr->rarity, r_ptr->d_char, r_ptr->name,
				r_ptr->power, r_ptr->scaled_power, r_ptr->melee_dam,
				r_ptr->spell_dam, r_ptr->hp);
		}

		file_close(mon_fp);
	}

	/* Free power array */
	FREE(power);

	/* Success */
	return 0;
}