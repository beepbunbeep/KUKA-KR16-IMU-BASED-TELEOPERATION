
classdef BLEJointReader < matlab.System

    properties (Access = private)
        BLEDevice
        BLECharacteristic
        LatestAngle = 0
    end

    methods (Access = protected)
        function outputSize = getOutputSizeImpl(~)
    outputSize = [1 1];
end

function outputType = getOutputDataTypeImpl(~)
    outputType = 'double';
end

function outputComplexity = isOutputComplexImpl(~)
    outputComplexity = false;
end

function fixedSize = isOutputFixedSizeImpl(~)
    fixedSize = true;
end

        function setupImpl(obj)

            % Initialize BLE connection
            obj.BLEDevice = ble("KUKA_ESP32_C3");

            obj.BLECharacteristic = characteristic( ...
                obj.BLEDevice, ...
                "4fafc201-1fb5-459e-8fcc-c5c9c331914b", ...
                "beb5483e-36e1-4688-b7f5-ea07361b26a8");

        end

        function angle = stepImpl(obj)

            % Read current joint angle
            rawData = read(obj.BLECharacteristic);

            valStr = char(rawData(:)');
            value = str2double(strtrim(valStr));

            if ~isnan(value)
                obj.LatestAngle = value;
            end

            angle = obj.LatestAngle;

        end

        function releaseImpl(obj)

            % Release BLE connection
            obj.BLEDevice = [];
            obj.BLECharacteristic = [];

        end

    end
end