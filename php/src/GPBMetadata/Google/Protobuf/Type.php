<?php
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# NO CHECKED-IN PROTOBUF GENCODE
# source: google/protobuf/type.proto

namespace GPBMetadata\Google\Protobuf;

class Type
{
    public static $is_initialized = false;

    public static function initOnce() {
        $pool = \Google\Protobuf\Internal\DescriptorPool::getGeneratedPool();

        if (static::$is_initialized == true) {
          return;
        }
        \GPBMetadata\Google\Protobuf\Any::initOnce();
        \GPBMetadata\Google\Protobuf\SourceContext::initOnce();
        $pool->internalAddGeneratedFile(
            "\x0A\xD4\x0C\x0A\x1Agoogle/protobuf/type.proto\x12\x0Fgoogle.protobuf\x1A\$google/protobuf/source_context.proto\"\xE8\x01\x0A\x04Type\x12\x0C\x0A\x04name\x18\x01 \x01(\x09\x12&\x0A\x06fields\x18\x02 \x03(\x0B2\x16.google.protobuf.Field\x12\x0E\x0A\x06oneofs\x18\x03 \x03(\x09\x12(\x0A\x07options\x18\x04 \x03(\x0B2\x17.google.protobuf.Option\x126\x0A\x0Esource_context\x18\x05 \x01(\x0B2\x1E.google.protobuf.SourceContext\x12'\x0A\x06syntax\x18\x06 \x01(\x0E2\x17.google.protobuf.Syntax\x12\x0F\x0A\x07edition\x18\x07 \x01(\x09\"\xD5\x05\x0A\x05Field\x12)\x0A\x04kind\x18\x01 \x01(\x0E2\x1B.google.protobuf.Field.Kind\x127\x0A\x0Bcardinality\x18\x02 \x01(\x0E2\".google.protobuf.Field.Cardinality\x12\x0E\x0A\x06number\x18\x03 \x01(\x05\x12\x0C\x0A\x04name\x18\x04 \x01(\x09\x12\x10\x0A\x08type_url\x18\x06 \x01(\x09\x12\x13\x0A\x0Boneof_index\x18\x07 \x01(\x05\x12\x0E\x0A\x06packed\x18\x08 \x01(\x08\x12(\x0A\x07options\x18\x09 \x03(\x0B2\x17.google.protobuf.Option\x12\x11\x0A\x09json_name\x18\x0A \x01(\x09\x12\x15\x0A\x0Ddefault_value\x18\x0B \x01(\x09\"\xC8\x02\x0A\x04Kind\x12\x10\x0A\x0CTYPE_UNKNOWN\x10\x00\x12\x0F\x0A\x0BTYPE_DOUBLE\x10\x01\x12\x0E\x0A\x0ATYPE_FLOAT\x10\x02\x12\x0E\x0A\x0ATYPE_INT64\x10\x03\x12\x0F\x0A\x0BTYPE_UINT64\x10\x04\x12\x0E\x0A\x0ATYPE_INT32\x10\x05\x12\x10\x0A\x0CTYPE_FIXED64\x10\x06\x12\x10\x0A\x0CTYPE_FIXED32\x10\x07\x12\x0D\x0A\x09TYPE_BOOL\x10\x08\x12\x0F\x0A\x0BTYPE_STRING\x10\x09\x12\x0E\x0A\x0ATYPE_GROUP\x10\x0A\x12\x10\x0A\x0CTYPE_MESSAGE\x10\x0B\x12\x0E\x0A\x0ATYPE_BYTES\x10\x0C\x12\x0F\x0A\x0BTYPE_UINT32\x10\x0D\x12\x0D\x0A\x09TYPE_ENUM\x10\x0E\x12\x11\x0A\x0DTYPE_SFIXED32\x10\x0F\x12\x11\x0A\x0DTYPE_SFIXED64\x10\x10\x12\x0F\x0A\x0BTYPE_SINT32\x10\x11\x12\x0F\x0A\x0BTYPE_SINT64\x10\x12\"t\x0A\x0BCardinality\x12\x17\x0A\x13CARDINALITY_UNKNOWN\x10\x00\x12\x18\x0A\x14CARDINALITY_OPTIONAL\x10\x01\x12\x18\x0A\x14CARDINALITY_REQUIRED\x10\x02\x12\x18\x0A\x14CARDINALITY_REPEATED\x10\x03\"\xDF\x01\x0A\x04Enum\x12\x0C\x0A\x04name\x18\x01 \x01(\x09\x12-\x0A\x09enumvalue\x18\x02 \x03(\x0B2\x1A.google.protobuf.EnumValue\x12(\x0A\x07options\x18\x03 \x03(\x0B2\x17.google.protobuf.Option\x126\x0A\x0Esource_context\x18\x04 \x01(\x0B2\x1E.google.protobuf.SourceContext\x12'\x0A\x06syntax\x18\x05 \x01(\x0E2\x17.google.protobuf.Syntax\x12\x0F\x0A\x07edition\x18\x06 \x01(\x09\"S\x0A\x09EnumValue\x12\x0C\x0A\x04name\x18\x01 \x01(\x09\x12\x0E\x0A\x06number\x18\x02 \x01(\x05\x12(\x0A\x07options\x18\x03 \x03(\x0B2\x17.google.protobuf.Option\";\x0A\x06Option\x12\x0C\x0A\x04name\x18\x01 \x01(\x09\x12#\x0A\x05value\x18\x02 \x01(\x0B2\x14.google.protobuf.Any*C\x0A\x06Syntax\x12\x11\x0A\x0DSYNTAX_PROTO2\x10\x00\x12\x11\x0A\x0DSYNTAX_PROTO3\x10\x01\x12\x13\x0A\x0FSYNTAX_EDITIONS\x10\x02B{\x0A\x13com.google.protobufB\x09TypeProtoP\x01Z-google.golang.org/protobuf/types/known/typepb\xF8\x01\x01\xA2\x02\x03GPB\xAA\x02\x1EGoogle.Protobuf.WellKnownTypesb\x06proto3"
        , true);

        static::$is_initialized = true;
    }
}

