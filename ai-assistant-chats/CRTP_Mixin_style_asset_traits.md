## Me
In AssetTraits can I associate a label (const char or string) with each `AssetDefinition`? Maybe a static member? Maybe some fancy template programmin approach...

## AI Assistant
Yes, you can definitely associate labels with each `AssetDefinition`! Here are a few approaches you can use:

## Approach 1: Static Member Function (Recommended)

Add a static member function to each specialization:

```c++
// Specializations for supported mappings
template <>
struct AssetTypeFor<asset::ShaderStageDefinition> {
  using type = asset::ShaderStage;
  static constexpr const char* label() { return "ShaderStage"; }
};
template <>
struct AssetTypeFor<asset::GraphicsProgramDefinition> {
  using type = asset::GraphicsProgram;
  static constexpr const char* label() { return "GraphicsProgram"; }
};
template <>
struct AssetTypeFor<asset::MaterialDefinition> {
  using type = asset::Material;
  static constexpr const char* label() { return "Material"; }
};
template <>
struct AssetTypeFor<asset::MeshDefinition> {
  using type = asset::Mesh;
  static constexpr const char* label() { return "Mesh"; }
};
```


Usage:
```c++
auto label = AssetTypeFor<asset::ShaderStageDefinition>::label(); // "ShaderStage"
```


## Approach 2: Static Constexpr Variable

Add a static constexpr member variable:

```c++
template <>
struct AssetTypeFor<asset::ShaderStageDefinition> {
  using type = asset::ShaderStage;
  static constexpr const char* label = "ShaderStage";
};
template <>
struct AssetTypeFor<asset::GraphicsProgramDefinition> {
  using type = asset::GraphicsProgram;
  static constexpr const char* label = "GraphicsProgram";
};
// ... etc
```


Usage:
```c++
auto label = AssetTypeFor<asset::ShaderStageDefinition>::label; // "ShaderStage"
```


## Approach 3: Separate Trait Template (Most Flexible)

Create a dedicated trait for labels:

```c++
// Add this after your existing traits
template <AssetDefinition TDefinition>
struct AssetLabelFor {
  // No default implementation
};

template <>
struct AssetLabelFor<asset::ShaderStageDefinition> {
  static constexpr const char* value = "ShaderStage";
};
template <>
struct AssetLabelFor<asset::GraphicsProgramDefinition> {
  static constexpr const char* value = "GraphicsProgram";
};
template <>
struct AssetLabelFor<asset::MaterialDefinition> {
  static constexpr const char* value = "Material";
};
template <>
struct AssetLabelFor<asset::MeshDefinition> {
  static constexpr const char* value = "Mesh";
};

// Helper alias
template <AssetDefinition TDefinition>
constexpr const char* AssetLabelFor_v = AssetLabelFor<TDefinition>::value;
```


Usage:
```c++
auto label = AssetLabelFor_v<asset::ShaderStageDefinition>; // "ShaderStage"
```


## Approach 4: Template Function (Generic Access)

You could also add a generic function that works with any asset definition:

```c++
template <AssetDefinition TDefinition>
constexpr const char* getAssetLabel() {
  if constexpr (std::is_same_v<TDefinition, asset::ShaderStageDefinition>) {
    return "ShaderStage";
  } else if constexpr (std::is_same_v<TDefinition, asset::GraphicsProgramDefinition>) {
    return "GraphicsProgram";
  } else if constexpr (std::is_same_v<TDefinition, asset::MaterialDefinition>) {
    return "Material";
  } else if constexpr (std::is_same_v<TDefinition, asset::MeshDefinition>) {
    return "Mesh";
  }
}
```


Usage:
```c++
auto label = getAssetLabel<asset::ShaderStageDefinition>(); // "ShaderStage"
```


I'd recommend **Approach 1** (static member function) or **Approach 3** (separate trait) as they're the most consistent with your existing design pattern and provide compile-time string literals that are efficient and type-safe.

## Me
Yeah, for consistency I can go with `AssetLabelFor` approach!

I've a question though, why you didn't add any constraints via concept in any of the approaches?

## AI Assistant
Great question! You're absolutely right to point that out. For consistency with the existing code pattern, I should have added the concept constraint. Here's the corrected version:

```c++
// Add this after your existing traits
template <AssetDefinition TDefinition>
struct AssetLabelFor {
  // No default implementation - will cause compile error for unsupported types
};

template <>
struct AssetLabelFor<asset::ShaderStageDefinition> {
  static constexpr const char* value = "ShaderStage";
};
template <>
struct AssetLabelFor<asset::GraphicsProgramDefinition> {
  static constexpr const char* value = "GraphicsProgram";
};
template <>
struct AssetLabelFor<asset::MaterialDefinition> {
  static constexpr const char* value = "Material";
};
template <>
struct AssetLabelFor<asset::MeshDefinition> {
  static constexpr const char* value = "Mesh";
};

// Helper alias with concept constraint
template <AssetDefinition TDefinition>
constexpr const char* AssetLabelFor_v = AssetLabelFor<TDefinition>::value;
```


The reason I should have included the `AssetDefinition` concept constraint is:

1. **Consistency**: Your existing traits (`AssetTypeFor`, `CacheTypeFor`, etc.) all use the `AssetDefinition` concept constraint
2. **Type Safety**: It ensures only valid asset definition types can be used
3. **Better Error Messages**: If someone tries to use an invalid type, they get a clear concept violation error rather than a confusing template instantiation error
4. **Self-Documenting**: The constraint makes it immediately clear what types are expected

The constraint is already on the primary template, and the helper alias `AssetLabelFor_v` inherits this constraint, so you get compile-time safety and consistent behavior with the rest of your trait system.

Thanks for catching that - it's a good example of how concepts improve code clarity and consistency!

## Me
I meant you to use

```c++
template <typename T>
concept AssetDefinition =
    std::is_same_v<T, asset::ShaderStageDefinition> || std::is_same_v<T, asset::GraphicsProgramDefinition> ||
    std::is_same_v<T, asset::MaterialDefinition> || std::is_same_v<T, asset::MeshDefinition>;
```


at the beginning of the file thouh.

## AI Assistant
Ah, I see what you mean! You're asking why I didn't update the `AssetDefinition` concept itself to include the new types, rather than just using the existing concept as a constraint.

Looking at your current concept definition:

```c++
template <typename T>
concept AssetDefinition =
    std::is_same_v<T, asset::ShaderStageDefinition> || std::is_same_v<T, asset::GraphicsProgramDefinition> ||
    std::is_same_v<T, asset::MaterialDefinition> || std::is_same_v<T, asset::MeshDefinition>;
```


The `AssetLabelFor` trait I suggested is already properly constrained by this existing concept - it will only work with the four asset definition types you've already defined. So there's no need to modify the concept itself.

However, if you were asking about a different approach where we modify the concept to somehow include label information, that would be a more complex design. The current approach keeps the concerns separated:

- The `AssetDefinition` concept defines what types are valid asset definitions
- The `AssetLabelFor` trait provides labels for those valid types

This separation is clean and follows the single responsibility principle. The concept remains focused on type validation, while the trait handles the labeling concern.

Did you have a different modification to the concept in mind, or were you just checking that I was using the existing concept constraint properly?

## Me
OK, let me ask a similar question then... Can I modify `concept AssetDefinition` so that each AssetDefinition automatically acquires a cache type. Something I can get by, say, `MeshDefinition::CacheType`. 

That way I don't have to remember  `CacheTypeFor` trait.

## AI Assistant
Unfortunately, you can't modify a concept to automatically add members to existing types. Concepts are just compile-time predicates that check if a type satisfies certain conditions - they don't modify the types themselves.

However, you have a few alternatives to achieve what you want:

## Option 1: Modify the Asset Definition Types Directly

Add the cache type as a member to each asset definition class:

```c++
// In your asset definition headers
struct ShaderStageDefinition {
    // ... existing members ...
    using CacheType = std::unordered_map<StableId<ShaderStageDefinition>, Handle<asset::ShaderStage>>;
};

struct GraphicsProgramDefinition {
    // ... existing members ...
    using CacheType = std::unordered_map<StableId<GraphicsProgramDefinition>, Handle<asset::GraphicsProgram>>;
};
// ... etc for other definitions
```


Then you can use `MeshDefinition::CacheType` directly.

## Option 2: Require Cache Type in the Concept

Modify your concept to require that each asset definition has a `CacheType` member:

```c++
template <typename T>
concept AssetDefinition = requires {
    typename T::CacheType;  // Require CacheType member
} && (std::is_same_v<T, asset::ShaderStageDefinition> || 
      std::is_same_v<T, asset::GraphicsProgramDefinition> ||
      std::is_same_v<T, asset::MaterialDefinition> || 
      std::is_same_v<T, asset::MeshDefinition>);
```


This would force you to add `CacheType` to each definition, but gives you the syntax you want.

## Option 3: Use a Shorter Alias

Keep your current approach but add a shorter alias:

```c++
template <AssetDefinition TDefinition>
using CacheType = CacheTypeFor_t<TDefinition>;

// Usage: CacheType<MeshDefinition> instead of CacheTypeFor_t<MeshDefinition>
```


**Option 1** is probably the cleanest if you control the asset definition types, as it makes the relationship explicit and gives you the exact syntax you wanted.

## Me
Can I somehow convert

```c++
template <AssetType TAsset>
struct AssetWithCacheType {
  using CacheType = std::unordered_map<StableId<TAsset>, Handle<TAsset>>;
};
```


into a concept? :-) I'm beginner in template metaprogramming, so I'm just trying out random stuff...

## AI Assistant
Great question! You're thinking creatively about templates, but concepts and class templates serve different purposes, so you can't directly convert that struct into a concept.

However, I can show you a few related approaches:

## What You Can't Do
You can't make a concept that "injects" members into existing types. Concepts are just compile-time checks - they don't modify types.

## What You Can Do

### Option 1: Use Your Struct as a Base Class
```c++
template <AssetType TAsset>
struct AssetWithCacheType {
  using CacheType = std::unordered_map<StableId<TAsset>, Handle<TAsset>>;
};

// Then inherit from it in your asset definitions
struct MeshDefinition : AssetWithCacheType<asset::Mesh> {
  // ... your existing members ...
  // Now you automatically have CacheType available
};
```


### Option 2: Create a Concept That Requires CacheType
```c++
template <typename T>
concept HasCacheType = requires {
  typename T::CacheType;  // Requires the type to have a CacheType member
};

template <typename T>
concept AssetDefinitionWithCache = AssetDefinition<T> && HasCacheType<T>;
```


### Option 3: Use CRTP (Curiously Recurring Template Pattern)
```c++
template <typename TDerived, AssetType TAsset>
struct AssetDefinitionBase {
  using CacheType = std::unordered_map<StableId<TDerived>, Handle<TAsset>>;
};

struct MeshDefinition : AssetDefinitionBase<MeshDefinition, asset::Mesh> {
  // ... your members ...
  // CacheType is now available as MeshDefinition::CacheType
};
```


Your instinct about wanting to avoid the `CacheTypeFor` trait is good - **Option 1** (inheritance) is probably the cleanest way to get `MeshDefinition::CacheType` syntax while keeping things simple!

## Me
Option 2 is not good, because it doesn't help with code-reuse for cache type definition per asset.

Base class sounds good but it introduces a hierarchy unnecessarily. Well... I guess I can think about it not as inheritance but as a mixin. Does that makes sense?

I'm a fan of CRTP. It does not introduce inheritance relationship, right? If yes, for this case it's not much different than option 1?



## AI Assistant
You're absolutely right on all counts! Let me address your observations:

## Mixin vs Inheritance
Yes, thinking of it as a **mixin** rather than inheritance makes perfect sense! Mixins are about adding functionality/capabilities to a class, not establishing an "is-a" relationship. Your asset definitions aren't inheriting behavior - they're just getting a common type definition mixed in.

## CRTP vs Simple Inheritance
You're correct that CRTP doesn't introduce a true inheritance relationship in the traditional OOP sense. However, in this specific case, CRTP and simple inheritance are functionally very similar:

**Simple Inheritance (Mixin approach):**
```c++
template <AssetType TAsset>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<???>, Handle<TAsset>>;  // Wait, what goes here?
};

struct MeshDefinition : AssetCacheTypeMixin<asset::Mesh> {
  // ... your members ...
};
```


**CRTP:**
```c++
template <typename TDerived, AssetType TAsset>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<TDerived>, Handle<TAsset>>;
};

struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh> {
  // ... your members ...
};
```


## The Key Difference
CRTP is actually **necessary** here because the cache type needs to know about the **definition type** (for `StableId<MeshDefinition>`), not just the asset type. With simple inheritance, you can't get access to the derived type.

So **CRTP is the right choice** for your use case! It gives you the mixin pattern without traditional inheritance semantics, and it solves the technical requirement of needing the derived type in the template.

Your instincts are spot on - CRTP is perfect for this kind of "add common functionality without inheritance hierarchy" scenario.

## Me
Can I squeeze the label somewhere here?

```
template <typename TDerived, AssetType TAsset>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<TDerived>, Handle<TAsset>>;
};

struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh> {
  // ... your members ...
};
```


Like, can I request to have a specific type for a template parameter? `template <typename TDerived, AssetType TAsset, std::string_view label>`

That'll reduce a lot of code!

## AI Assistant
Great idea! You're thinking in the right direction, but there are some technical limitations with `std::string_view` as a template parameter.

## What You Can Do

**Option 1: Use a `const char*` (C++11+)**
```c++
template <typename TDerived, AssetType TAsset, const char* Label>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<TDerived>, Handle<TAsset>>;
  static constexpr const char* label = Label;
};

// Usage requires external linkage variables:
constexpr const char mesh_label[] = "Mesh";
struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, mesh_label> {
  // ... your members ...
};
```


**Option 2: Use a fixed_string wrapper (C++20)**
```c++
template <size_t N>
struct fixed_string {
  char data[N];
  constexpr fixed_string(const char (&str)[N]) {
    std::copy_n(str, N, data);
  }
  constexpr operator std::string_view() const { return {data, N-1}; }
};

template <typename TDerived, AssetType TAsset, fixed_string Label>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<TDerived>, Handle<TAsset>>;
  static constexpr std::string_view label = Label;
};

// Clean usage:
struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, "Mesh"> {
  // ... your members ...
};
```


## What You Can't Do (Yet)
`std::string_view` as a non-type template parameter is only available in **C++26** (very cutting edge).

## Recommendation
Since you're using C++23, **Option 2** with `fixed_string` is probably your best bet if you want clean syntax. It's a common pattern in modern C++ for string literals as template parameters.

Would you like me to show you the complete implementation of the `fixed_string` approach?

## Me
In option 1, why can't I just give a string literal?  `struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, "Mesh">`

What if I use std::string?

## AI Assistant
Great questions! Let me explain the technical reasons:

## Why String Literals Don't Work Directly

```c++
// This WON'T compile:
struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, "Mesh">
```


The issue is that **string literals don't have external linkage**. Each time you write `"Mesh"`, the compiler creates a new anonymous array, and template parameters need to refer to the *same* object. The compiler can't guarantee that `"Mesh"` in one place is the same memory location as `"Mesh"` elsewhere.

That's why you need:
```c++
// This DOES work:
constexpr const char mesh_label[] = "Mesh";  // External linkage
struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, mesh_label>
```


## Why `std::string` Won't Work

```c++
template <typename TDerived, AssetType TAsset, std::string Label>  // ❌ Won't compile
```


Non-type template parameters have strict requirements. They must be:
- Integral types (`int`, `char`, etc.)
- Pointer/reference types
- Enumeration types
- `std::nullptr_t`
- And a few others...

`std::string` is a **class type** with dynamic memory allocation, so it can't be a template parameter. The compiler needs to know the exact value at compile time, and `std::string` is too complex for that.

## The Solution
This is exactly why the `fixed_string` wrapper exists - it converts string literals into a compile-time-friendly type that can be used as a template parameter.

The fundamental issue is that C++ templates were designed before modern string handling, so we need these workarounds until C++26 adds proper `std::string_view` support for template parameters.

## Me
Oh, I see! fixed string converts a text into char[] at compile time.

## AI Assistant
Exactly! You've got it perfectly! 🎯

The `fixed_string` is essentially a compile-time wrapper that:

1. **Takes a string literal** like `"Mesh"`
2. **Converts it into a `char[N]` array** at compile time 
3. **Stores the size `N` in the type** so the template system can work with it
4. **Provides conversion back to `std::string_view`** for easy usage

So when you write:
```c++
AssetCacheTypeMixin<MeshDefinition, asset::Mesh, "Mesh">
```


The `fixed_string` template deduction guides figure out that `"Mesh"` is `char[5]` (including null terminator), creates a `fixed_string<5>`, and stores the characters in a compile-time accessible way.

It's basically a "compile-time string" that the template system can understand and work with, bridging the gap between string literals and the template parameter requirements.

Pretty clever workaround for the limitations of the current template system! And it gives you that clean syntax you wanted without the verbose external variable declarations.

## Me
Then I can also have a 4th template parameter, `AssetDefinition TDefinition`, so set the definition type corresponding to this asset. That way I won't need  `AssetTypeFor` traits! It'll be more local and compact.

## AI Assistant
Absolutely brilliant idea! 🎯 That's a perfect way to make everything more self-contained and eliminate the need for separate trait mappings.

Your mixin would become:

```c++
template <typename TDerived, AssetType TAsset, fixed_string Label, AssetDefinition TDefinition>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<TDerived>, Handle<TAsset>>;
  using AssetType = TAsset;
  using DefinitionType = TDefinition;
  static constexpr std::string_view label = Label;
};

struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, "Mesh", MeshDefinition> {
  // ... your members ...
};
```


Wait, there's a subtle issue - `MeshDefinition` appears on both sides! But that's actually fine with CRTP, it just looks a bit circular. 

Alternatively, you could flip it and make the definition type the primary template parameter:

```c++
template <AssetDefinition TDefinition, AssetType TAsset, fixed_string Label>
struct AssetCacheTypeMixin {
  using CacheType = std::unordered_map<StableId<TDefinition>, Handle<TAsset>>;
  using AssetType = TAsset;
  using DefinitionType = TDefinition;
  static constexpr std::string_view label = Label;
};

struct MeshDefinition : AssetCacheTypeMixin<MeshDefinition, asset::Mesh, "Mesh"> {
  // ... your members ...
};
```


This way you get:
- ✅ No separate `AssetTypeFor_t` traits needed
- ✅ Everything is local and compact
- ✅ Clean, readable syntax
- ✅ All the type information is right there in the inheritance

Much cleaner than having separate trait specializations scattered around!

## Me
Actually I'd like to keep the definitions pure, just simple structs. And do the CRTP on assets. 

Or, I can do CRTP on definitions, only to set the corresponding asset to make `DefinitionTypeFor` obselete.

## AI Assistant
Ah, I see what you're going for! You want to keep the definitions as pure data structs and use CRTP to establish the relationships. That's a clean separation of concerns.

Here's how you could do CRTP on the definitions to eliminate `DefinitionTypeFor`:

```c++
template <typename TDerived, AssetType TAsset, fixed_string Label>
struct AssetDefinitionMixin {
  using AssetType = TAsset;
  static constexpr std::string_view label = Label;
};

// Pure data definitions with CRTP inheritance
struct MeshDefinition : AssetDefinitionMixin<MeshDefinition, asset::Mesh, "Mesh"> {
  // ... just your data members, no extra complexity
  std::string filePath;
  // etc.
};

struct MaterialDefinition : AssetDefinitionMixin<MaterialDefinition, asset::Material, "Material"> {
  // ... just your data members
  std::string shaderProgram;
  // etc.
};
```


Then your traits become much simpler:

```c++
template <AssetDefinition TDefinition>
using AssetTypeFor_t = typename TDefinition::AssetType;

// DefinitionTypeFor becomes obsolete - you already have the definition!
```


This approach:
- ✅ Keeps definitions as simple data structs
- ✅ Eliminates the need for `DefinitionTypeFor` mappings
- ✅ Makes the asset-definition relationship explicit and local
- ✅ Reduces boilerplate trait specializations
- ✅ Still gives you the label for debugging/logging

The beauty is that each definition declares its own asset type right in its inheritance, so there's no separate mapping to maintain. Much cleaner!

## Me
My concepts already use these derived types though:

```c++
template <typename TDefinition>
concept AssetDefinition = std::is_same_v<TDefinition, asset::ShaderStageDefinition> ||
                          std::is_same_v<TDefinition, asset::GraphicsProgramDefinition> ||
                          std::is_same_v<TDefinition, asset::MaterialDefinition> ||
                          std::is_same_v<TDefinition, asset::MeshDefinition>;

template <typename TAsset>
concept AssetType =
    std::is_same_v<TAsset, asset::GraphicsProgram> || std::is_same_v<TAsset, asset::ShaderStage> ||
    std::is_same_v<TAsset, asset::Material> || std::is_same_v<TAsset, asset::Mesh>;
```


Does that cause some circularity?

## AI Assistant
Good catch! Yes, there's a potential circularity issue here. Your current concepts are **explicitly listing the concrete types**, so if those types now inherit from a mixin, you need to be careful about the dependency order.

Here are a few solutions:

## Option 1: Keep the explicit concept (safest)
Just keep your current concept as-is. It works fine and there's no circularity because the concept doesn't depend on the mixin - it just lists the final concrete types:

```c++
template <typename TDefinition>
concept AssetDefinition = std::is_same_v<TDefinition, asset::ShaderStageDefinition> ||
                          std::is_same_v<TDefinition, asset::GraphicsProgramDefinition> ||
                          // ... etc
```


The CRTP mixin is just an implementation detail that doesn't affect the concept.

## Option 2: Use a trait-based concept 
If you want the concept to be more generic, you could change it to check for the presence of the mixin:

```c++
template <typename TDefinition>
concept AssetDefinition = requires {
  typename TDefinition::AssetType;
  { TDefinition::label } -> std::convertible_to<std::string_view>;
} && AssetType<typename TDefinition::AssetType>;
```


This would automatically work with any type that inherits from your mixin.

## Option 3: Hybrid approach
Keep the explicit list for now, but add a static_assert in the mixin to ensure consistency:

```c++
template <typename TDerived, AssetType TAsset, fixed_string Label>
struct AssetDefinitionMixin {
  using AssetType = TAsset;
  static constexpr std::string_view label = Label;
  
  static_assert(AssetDefinition<TDerived>, "Derived type must be in AssetDefinition concept");
};
```


I'd recommend **Option 1** - your current approach is clear, explicit, and avoids any potential circularity issues. The CRTP mixin can coexist perfectly with your explicit concept.

## Me
Oh, so the `concept AssetDefinition` does not require `asset::ShaderStageDefinition` to be defined before itself?

## AI Assistant
Exactly! The `concept AssetDefinition` **does** require `asset::ShaderStageDefinition` to be declared/defined before the concept can be used, but there's no circularity with CRTP because:

1. **Forward declaration is enough for `std::is_same_v`**: The concept only needs the type to be declared (not fully defined) to use it in `std::is_same_v`

2. **CRTP happens at definition time**: When you define the actual struct, that's when CRTP kicks in:

```c++
// 1. Forward declare (this is enough for the concept)
struct ShaderStageDefinition;

// 2. Define the concept (works with just forward declaration)
template <typename TDefinition>
concept AssetDefinition = std::is_same_v<TDefinition, asset::ShaderStageDefinition> ||
                          /* ... other types ... */;

// 3. Later, define the actual struct with CRTP
struct ShaderStageDefinition : AssetDefinitionMixin<ShaderStageDefinition, asset::ShaderStage, "Shader"> {
    // data members...
};
```


The key insight is that `std::is_same_v` only needs the **type identity**, not the full definition. So as long as `ShaderStageDefinition` is declared somewhere before the concept, there's no circularity.

The CRTP inheritance happens when you actually define the struct, which comes after both the forward declaration and the concept definition. No circular dependency! 🎯
