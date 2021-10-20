/*
 * Copyright (C) 2021 CESNET, https://photonics.cesnet.cz/
 *
 * Written by Václav Kubernát <kubernat@cesnet.cz>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

const auto example_schema = R"(
module example-schema {
    yang-version 1.1;
    namespace "http://example.com/";
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
        }
    }

    anydata myData {
    }
})";

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
)";

const auto example_schema3 = R"(
module example-schema3 {
    yang-version 1.1;
    namespace "http://example3.com/";
    prefix rofl;

    leaf-list values {
        ordered-by user;
        type int32;
    }

    list person {
        key 'name';
        leaf name {
            type string;
        }
    }
}
)";
