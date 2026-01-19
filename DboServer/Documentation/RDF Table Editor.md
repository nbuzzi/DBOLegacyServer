# RDF Table Editor (NET 8)

A lightweight Windows Forms tool to view and edit the game's RDF-based tables.

Status: MVP with binary RDF support via schema-driven IO. Currently recognized schemas: CharTitle and ChatCommand (margin + fixed records). CSV fallback remains for testing.

## Build

- Requires .NET SDK 8 (or newer SDK with net8 targeting pack)
- Build with your IDE or from repo root:

```
dotnet build Tools/TableEditor/Tools/RdfTableEditor/RdfTableEditor.csproj -c Release
```

## Run

```
dotnet run --project Tools/TableEditor/Tools/RdfTableEditor/RdfTableEditor.csproj
```

## Usage

- File > Open… loads a table file (.rdf).
- Edit cells directly.
- File > Save / Save As… writes back using the placeholder serializer.

## Roadmap

- Add schemas for core tables (Mob/Npc/Item/SystemEffect/Skill/Spawn).
- Custom readers for variable-length formats (e.g., config/text tables with length-prefixed UTF-16 strings).
- Auto-generate grid columns from schema with validation and enum renderers.
- Multi-table navigation, search, diff, CSV import/export, undo/redo.
