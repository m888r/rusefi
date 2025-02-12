/*
 * @file test_ignition_scheduling.cpp
 *
 * @date Nov 17, 2019
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "pch.h"
#include "spark_logic.h"

using ::testing::_;

TEST(ignition, twoCoils) {
	EngineTestHelper eth(FRANKENSO_BMW_M73_F);

	// let's recalculate with zero timing so that we can focus on relation advance between cylinders
	setArrayValues(engine->engineState.timingAdvance, 0.0f);
	initializeIgnitionActions();

	// first one to fire uses first coil
	EXPECT_EQ(engine->ignitionEvents.elements[0].cylinderNumber, 0);
	EXPECT_EQ(engine->ignitionEvents.elements[1].cylinderNumber, 6);
	EXPECT_EQ(engine->ignitionEvents.elements[2].cylinderNumber, 0);
	EXPECT_EQ(engine->ignitionEvents.elements[3].cylinderNumber, 6);
	EXPECT_EQ(engine->ignitionEvents.elements[4].cylinderNumber, 0);
	EXPECT_EQ(engine->ignitionEvents.elements[5].cylinderNumber, 6);
	EXPECT_EQ(engine->ignitionEvents.elements[6].cylinderNumber, 0);
	EXPECT_EQ(engine->ignitionEvents.elements[7].cylinderNumber, 6);
	EXPECT_EQ(engine->ignitionEvents.elements[8].cylinderNumber, 0);
	EXPECT_EQ(engine->ignitionEvents.elements[9].cylinderNumber, 6);
	EXPECT_EQ(engine->ignitionEvents.elements[10].cylinderNumber, 0);
	EXPECT_EQ(engine->ignitionEvents.elements[11].cylinderNumber, 6);

	ASSERT_EQ(engine->ignitionEvents.elements[0].sparkAngle, 0);
	ASSERT_EQ((void*)engine->ignitionEvents.elements[0].outputs[0], (void*)&enginePins.coils[0]);


	ASSERT_EQ(engine->ignitionEvents.elements[1].sparkAngle, 720 / 12);
	ASSERT_EQ((void*)engine->ignitionEvents.elements[1].outputs[0], (void*)&enginePins.coils[6]);

	ASSERT_EQ(engine->ignitionEvents.elements[3].sparkAngle, 3 * 720 / 12);
	ASSERT_EQ((void*)engine->ignitionEvents.elements[3].outputs[0], (void*)&enginePins.coils[6]);
}

TEST(ignition, trailingSpark) {
	EngineTestHelper eth(TEST_ENGINE);
	engineConfiguration->isFasterEngineSpinUpEnabled = false;

	/**
	// TODO #3220: this feature makes this test sad, eventually remove this line (and the ability to disable it altogether)
	 * I am pretty sure that it's about usage of improper method clearQueue() below see it's comment
	 */
	engine->enableOverdwellProtection = false;

	EXPECT_CALL(*eth.mockAirmass, getAirmass(_))
		.WillRepeatedly(Return(AirmassResult{0.1008f, 50.0f}));

	setupSimpleTestEngineWithMafAndTT_ONE_trigger(&eth);
	engineConfiguration->specs.cylindersCount = 1;
	engineConfiguration->specs.firingOrder = FO_1;
	engineConfiguration->isInjectionEnabled = false;
	engineConfiguration->isIgnitionEnabled = true;

	// Fire trailing spark 10 degrees after main spark
	engine->engineState.trailingSparkAngle = 10;

	engineConfiguration->injectionMode = IM_SEQUENTIAL;

	setWholeTimingTable(0);

	eth.fireTriggerEventsWithDuration(20);
	// still no RPM since need to cycles measure cycle duration
	eth.fireTriggerEventsWithDuration(20);
	ASSERT_EQ( 3000,  Sensor::getOrZero(SensorType::Rpm)) << "RPM#0";
	eth.clearQueue();

	/**
	 * Trigger up - scheduling fuel for full engine cycle
	 */
	eth.fireRise(20);

	// Primary coil should be high
	EXPECT_EQ(enginePins.coils[0].getLogicValue(), true);
	EXPECT_EQ(enginePins.trailingCoils[0].getLogicValue(), false);

	// Should be a TDC callback + spark firing
	EXPECT_EQ(engine->executor.size(), 2);

	// execute all actions
	eth.executeActions();

	// Primary and secondary coils should be low - primary just fired
	EXPECT_EQ(enginePins.coils[0].getLogicValue(), false);
	EXPECT_EQ(enginePins.trailingCoils[0].getLogicValue(), false);

	// Now enable trailing sparks
	engineConfiguration->enableTrailingSparks = true;

	// Fire trigger fall - should schedule ignition chargings (rising edges)
	eth.fireFall(20);
	eth.moveTimeForwardMs(18);
	eth.executeActions();

	// Primary low, scheduling trailing
	EXPECT_EQ(enginePins.coils[0].getLogicValue(), true);
	EXPECT_EQ(enginePins.trailingCoils[0].getLogicValue(), false);

	eth.moveTimeForwardMs(2);
	eth.executeActions();

	// and secondary coils should be low
	EXPECT_EQ(enginePins.trailingCoils[0].getLogicValue(), true);

	// Fire trigger rise - should schedule ignition firings
	eth.fireRise(0);
	eth.moveTimeForwardMs(1);
	eth.executeActions();

	// Primary goes low, scheduling trailing
	EXPECT_EQ(enginePins.coils[0].getLogicValue(), false);
	EXPECT_EQ(enginePins.trailingCoils[0].getLogicValue(), true);

	eth.moveTimeForwardMs(1);
	eth.executeActions();
	// secondary coils should be low
	EXPECT_EQ(enginePins.trailingCoils[0].getLogicValue(), false);
}

TEST(ignition, CylinderTimingTrim) {
	EngineTestHelper eth(TEST_ENGINE);

	// Base timing 15 degrees
	setTable(config->ignitionTable, 15);

	// negative numbers retard timing, positive advance
	setTable(config->ignTrims[0].table, -4);
	setTable(config->ignTrims[1].table, -2);
	setTable(config->ignTrims[2].table,  2);
	setTable(config->ignTrims[3].table,  4);

	// run the ignition math
	engine->periodicFastCallback();

	// Check that each cylinder gets the expected timing
	float unadjusted = 15;
	EXPECT_NEAR(engine->engineState.timingAdvance[0], unadjusted - 4, EPS4D);
	EXPECT_NEAR(engine->engineState.timingAdvance[1], unadjusted - 2, EPS4D);
	EXPECT_NEAR(engine->engineState.timingAdvance[2], unadjusted + 2, EPS4D);
	EXPECT_NEAR(engine->engineState.timingAdvance[3], unadjusted + 4, EPS4D);
}
