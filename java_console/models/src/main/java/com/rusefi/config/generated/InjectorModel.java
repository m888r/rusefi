package com.rusefi.config.generated;

// this file was generated automatically by rusEFI tool ConfigDefinition.jar based on (unknown script) controllers/algo/fuel/injector_model.txt Mon Mar 06 04:15:17 UTC 2023

// by class com.rusefi.output.FileJavaFieldsConsumer
import com.rusefi.config.*;

public class InjectorModel {
	public static final Field M_DEADTIME = Field.create("M_DEADTIME", 0, FieldType.FLOAT).setBaseOffset(904);
	public static final Field PRESSUREDELTA = Field.create("PRESSUREDELTA", 4, FieldType.FLOAT).setBaseOffset(904);
	public static final Field PRESSURERATIO = Field.create("PRESSURERATIO", 8, FieldType.FLOAT).setBaseOffset(904);
	public static final Field[] VALUES = {
	M_DEADTIME,
	PRESSUREDELTA,
	PRESSURERATIO,
	};
}
