/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <string>

using namespace std::literals;

const auto example_schema = R"(
module example-schema {
    yang-version 1.1;
    namespace "http://example.com/coze";
    prefix coze;

    leaf dummy {
        type string;
    }

    leaf leafInt8 {
        description "A 8-bit integer leaf.";
        type int8;
    }

    leaf leafInt16 {
        description "A 16-bit integer leaf.";
        type int16;
    }

    leaf leafInt32 {
        description "A 32-bit integer leaf.";
        type int32;
    }

    leaf leafInt64 {
        description "A 64-bit integer leaf.";
        type int64;
    }

    leaf leafUInt8 {
        description "A 8-bit unsigned integer leaf.";
        type uint8;
    }

    leaf leafUInt16 {
        description "A 16-bit unsigned integer leaf.";
        type uint16;
    }

    leaf leafUInt32 {
        description "A 32-bit unsigned integer leaf.";
        type uint32;
    }

    leaf leafUInt64 {
        description "A 64-bit unsigned integer leaf.";
        type uint64;
    }

    leaf leafBool {
        description "A boolean.";
        type boolean;
    }

    leaf leafString {
        description "A string.";
        type string;
    }

    leaf leafEmpty {
        description "An `empty` leaf.";
        type empty;
    }

    leaf leafDecimal {
        description "A decimal value.";
        type decimal64 {
            fraction-digits 5;
        }
    }

    leaf leafBinary {
        description "A binary value.";
        type binary;
    }

    leaf intOrString {
        description "An int or a string.";
        type union {
            type int32;
            type string;
        }
    }

    list person {
        key 'name';
        leaf name {
            type string;
        }

        notification event {
            leaf description {
                type string;
            }
        }

        action poke { }
    }

    leaf bossPerson {
        type leafref {
            path '../person/name';
        }
    }

    leaf targetInstance {
        type instance-identifier;
    }

    leaf NOtargetInstance {
        type instance-identifier {
            require-instance false;
        }
    }

    leaf active {
        type boolean;
    }

    leaf flagBits {
        type bits {
            bit carry;
            bit sign;
            bit overflow;
        }
    }

    leaf pizzaSize {
        type enumeration {
            enum large;
            enum medium;
            enum small;
        }
    }

    identity food {
    }

    identity fruit {
        base "food";
    }

    identity pizza {
        base "food";
    }

    identity hawaii {
        base "pizza";
    }

    typedef foodTypedef {
        type identityref {
            base food;
        }
    }

    leaf leafFoodTypedef {
        type foodTypedef;
    }

    leaf pwnedUnion {
        type union {
            type decimal64 {
                fraction-digits 1;
            }
            type leafref {
                path '../person/name';
            }
            type union {
                type instance-identifier;
                type identityref {
                    base food;
                }
                type union {
                    type binary {
                        length 1;
                    }
                    type decimal64 {
                        fraction-digits 3;
                    }
                }
            }
        }
    }


    container first {
        container second {
            container third {
                container fourth {
                    leaf fifth {
                        type string;
                    }
                }
            }
        }
    }

    container presenceContainer {
        presence true;
    }

    container bigTree {
        container one {
            leaf myLeaf {
                type string;
            }
        }

        container two {
            list myList {
                key 'thekey';

                leaf thekey {
                    type int32;
                }
            }
        }
    }

    typedef myTypeInt {
        type int32;
        description "An int32 typedef.";
    }

    leaf typedefedLeafInt {
        type myTypeInt;
    }

    rpc myRpc {
        input {
            leaf inputLeaf {
                type string;
            }
        }

        output {
            leaf outputLeaf {
                type string;
            }
            leaf another {
                type string;
            }
        }
    }

    anydata myData {
    }

    anyxml ax {
    }

    notification event {
        description "Example notification event.";
        leaf event-class {
            type string;
            description "Event class identifier.";
        }
    }
})"s;

const auto example_schema2 = R"(
module example-schema2 {
    yang-version 1.1;
    namespace "http://example2.com/";
    prefix lol;
    container contWithTwoNodes {
        presence true;
        leaf one {
            type int32;
        }

        leaf two {
            type int32;
        }
    }
}
)"s;

const auto example_schema3 = R"(
module example-schema3 {
    yang-version 1.1;
    namespace "http://example3.com/";
    prefix rofl;

    leaf-list values {
        ordered-by user;
        type int32;
    }

    leaf-list valuesOrderedBySystem {
        type int32;
    }

    list person {
        key 'name';
        leaf name {
            type string;
        }
    }

    leaf leafWithDefault {
        type string;
        default "AHOJ";
    }
}
)"s;

const auto example_schema4 = R"(
module example-schema4 {
    yang-version 1.1;
    namespace "http://example3.com/";
    prefix yay;

    import example-schema {
      prefix "coze";
    }

    identity pizza {
      base "coze:pizza";
    }

    identity carpaccio {
      base "coze:pizza";
    }

    identity another-carpaccio {
      base "coze:pizza";
      base "pizza";
    }
}
)"s;

const auto example_schema5 = R"(
module example-schema5 {
    yang-version 1.1;
    namespace "http://example.com/5";
    prefix e5;

    container x {
        leaf x_a {
          type int32;
        }

        container x_b {
          leaf x_b_leaf {
            type int32;
          }

          container x_b_cont1 {
            leaf aaa {
              type int32;
            }
          }
        }
    }
}
)"s;

const auto type_module = R"(
module type_module {
    yang-version 1.1;
    namespace "http://example.com/custom-prefix";
    prefix custom-prefix;

    anydata anydataBasic {
    }

    anydata anydataWithMandatoryChild {
        mandatory true;
    }

    anyxml anyxmlBasic {
    }

    anyxml anyxmlWithMandatoryChild {
        mandatory true;
    }

    container choiceBasicContainer {
        choice choiceBasic {
            case case1 {
                leaf choiceBasicLeaf1 {
                    type string;
                }

                leaf-list choiceBasicLeafList1 {
                    type string;
                    ordered-by user;
                }
            }
            case case2 {
                leaf choiceBasicLeaf2 {
                    type string;
                }
            }
        }
    }

    container choiceWithMandatoryContainer {
        choice choiceWithMandatory {
            mandatory true;
            case case1 {
                leaf choiceWithMandatoryLeaf1 {
                    type string;
                }
            }
            case case2 {
                leaf choiceWithMandatoryLeaf2 {
                    type string;
                }
            }
        }
    }

    container choiceWithDefaultContainer {
        choice choiceWithDefault {
            default case1;
            case case1 {
                leaf choiceWithDefaultLeaf1 {
                    type string;
                }
            }
            case case2 {
                leaf choiceWithDefaultLeaf2 {
                    type string;
                }
            }
        }
    }

    container choiceWithoutCaseContainer {
        choice choiceWithoutCase {
            leaf choiceWithoutCaseLeaf1 {
                type string;
            }
        }
    }


    leaf leafBinary {
        type binary;
    }

    leaf leafBits {
        type bits {
            bit one;
            bit two;
            bit three;
        }
    }

    leaf leafEnum {
        type enumeration {
            enum A {
                value 2;
            }

            enum B {
                value 5;
            }
        }
    }

    leaf leafEnum2 {
        type enumeration {
            enum A;
            enum B;
        }
    }

    leaf leafNumber {
        type int32;
    }

    leaf leafRef {
        type leafref {
            path "/custom-prefix:listAdvancedWithOneKey/lol";
        }
    }

    leaf leafRefRelaxed {
        type leafref {
            path "/custom-prefix:listAdvancedWithOneKey/lol";
            require-instance false;
        }
    }

    leaf leafString {
        type string;
    }

    leaf leafUnion {
        type union {
            type string;
            type int32;
            type boolean;
        }
    }

    identity food;

    identity fruit {
        base food;
    }

    identity apple {
        base fruit;
    }

    identity meat {
        base food;
    }

    identity fruit-and-meat-mix {
        base fruit;
        base meat;
    }

    leaf meal {
        type identityref {
            base food;
        }
    }

    leaf leafWithConfigFalse {
        config false;
        type string;
    }

    leaf leafWithDefaultValue {
        type string;
        default "custom-prefix";
    }

    leaf leafWithDescription {
        type string;
        description "This is a description.";
    }

    leaf leafWithMandatoryTrue {
        mandatory true;
        type string;
    }

    leaf leafWithStatusDeprecated {
        status deprecated;
        type string;
    }

    leaf leafWithStatusObsolete {
        status obsolete;
        type string;
    }

    leaf leafWithUnits {
        type int32;
        units "s";
    }

    leaf iid-valid {
        type instance-identifier;
    }

    leaf iid-relaxed {
        type instance-identifier {
            require-instance false;
        }
    }

    leaf-list leafListBasic {
        type string;
        ordered-by user;
    }

    leaf-list leafListWithMinMaxElements {
        type int32;
        min-elements 1;
        max-elements 5;
    }

    leaf-list leafListWithUnits {
        type int32;
        units "s";
    }

    list listBasic {
        key 'primary-key';
        ordered-by user;

        leaf primary-key {
            type string;
        }
    }

    list listAdvancedWithOneKey {
        key 'lol';
        leaf lol {
            type string;
        }

        leaf notKey1 {
            type string {
              length "10 .. 20 | 50 .. 100 | 255";
              pattern "fo+";
              pattern "XXX" {
                description "yay";
                error-app-tag "x-XXX-failed";
                error-message "hard to fail this one";
                modifier invert-match;
              }
            }
        }

        leaf notKey2 {
            type string {
                length "min .. max" {
                    description "yay";
                    error-app-tag "x-XXX-failed";
                    error-message "hard to fail this one";
              }
            }
        }

        leaf notKey3 {
            type binary {
              length "10 .. 20 | 50 .. 100 | 255";
            }
        }

        leaf notKey4 {
            type binary {
                length "min .. max" {
                    description "yay";
                    error-app-tag "x-XXX-failed";
                    error-message "hard to fail this one";
              }
            }
        }
    }

    list listAdvancedWithTwoKey {
        key 'first second';
        leaf first {
            type string;
        }

        leaf second {
            type string;
        }
    }

    list listWithMinMaxElements {
        key 'primary-key';
        min-elements 1;
        max-elements 5;

        leaf primary-key {
            type string;
        }
    }

    container numeric {
        leaf i8 {
            type int8 {
                range "-20..-10 | -5 | 10..100" {
                    description "yay";
                    error-app-tag "this-is-a-nasty-limit";
                    error-message "flip a coin next time";
                }
            }
        }

        leaf i16 {
            type int16 {
                range "253..254";
            }
        }

        leaf i32 {
            type int32 {
                range "253..254";
            }
        }

        leaf i64 {
            type int64 {
                range "253..254";
            }
        }

        leaf deci {
            type decimal64 {
                fraction-digits 6;
                range "-123.456..666.042";
            }
        }

        leaf deci1 {
            type decimal64 {
                fraction-digits 1;
            }
        }

        leaf u8 {
            type uint8 {
                range "253..254";
            }
        }

        leaf u16 {
            type uint16 {
                range "253..254";
            }
        }

        leaf u32 {
            type uint32 {
                range "253..254";
            }
        }

        leaf u64 {
            type uint64 {
                range "253..254";
            }
        }
    }

    grouping grp-with-when {
        leaf x2 {
            when "666";
            type string;
        }
        action aaa {
        }
    }

    container container {
        when "1" {
            description "always on";
        }
        container x {
            when "2";
            leaf x1 {
                type string;
            }
            uses grp-with-when {
                when "3";
            }
        }
        container y {
        }
        container z {
            leaf z1 {
                type string;
            }
        }
    }

    container containerWithMandatoryChild {
        leaf leafWithMandatoryTrue {
            mandatory true;
            type string;
        }
    }
}
)"s;

const auto with_inet_types_module = R"(
module with-inet-types {
  yang-version 1.1;
  prefix "wit";
  namespace "wit";
  import ietf-inet-types {
    prefix "inet";
  }

  leaf hostname {
    type inet:host;
  }
}
)"s;

const auto with_extensions_module = R"(
module with-extensions {
  yang-version 1.1;
  prefix "we";
  namespace "we";
  import ietf-netconf-acm {
    prefix "nacm";
  }
  extension annotation {
      argument name;
      description "This is inspired by md:annotation";
  }
  container c {
    nacm:default-deny-write;
    we:annotation last-modified {
      type yang:date-and-time;
    }
  }
}
)"s;
const auto augmented_extensions_module = R"(
module augmenting-extensions {
  yang-version 1.1;
  prefix "ae";
  namespace "ae";
  import with-extensions {
    prefix "we";
  }
  extension another-annotation {
    we:annotation wtf-is-this;
  }
  augment "/we:c" {
    we:annotation last-modified {
      ae:another-annotation {
      }
    }
  }
}
)"s;
