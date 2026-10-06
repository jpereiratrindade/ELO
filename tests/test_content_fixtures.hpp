#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace elo::test {

inline std::filesystem::path create_test_content_fixture() {
    auto dir = std::filesystem::temp_directory_path() /
               ("elo_test_fixture_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    std::filesystem::create_directories(dir / "catalog" / "atoms");
    std::filesystem::create_directories(dir / "catalog" / "relations");
    std::filesystem::create_directories(dir / "catalog" / "recipes");
    std::filesystem::create_directories(dir / "catalog" / "variants");
    std::filesystem::create_directories(dir / "assets" / "images");
    std::filesystem::create_directories(dir / "assets" / "audio");
    std::filesystem::create_directories(dir / "sources");

    // manifest.json
    {
        std::ofstream f(dir / "manifest.json");
        f << R"({
  "bundle_id": "test_bundle_pampa",
  "version": "1.0.0",
  "title": "ELO Test Bundle",
  "default_theme": "pampa",
  "description": "Fixtures de teste herméticas para ELO"
})";
    }

    // sources.json
    {
        std::ofstream f(dir / "sources" / "sources.json");
        f << R"([
  {
    "source_id": "source_icmbio_2018",
    "title": "Livro Vermelho da Fauna Brasileira Ameaçada de Extinção",
    "authority": "ICMBio / MMA",
    "year": 2018,
    "confidence_level": "official"
  },
  {
    "source_id": "source_ufrgs_2023",
    "title": "Laboratório de Ecologia Vegetal e Pastagens Naturais",
    "authority": "UFRGS",
    "year": 2023,
    "confidence_level": "scientific"
  }
])";
    }

    // 1. species_cardeal_001
    {
        std::ofstream f(dir / "catalog" / "atoms" / "species_cardeal_001.json");
        f << R"({
  "schema_version": "0.1",
  "content_id": "species_cardeal_001",
  "type": "entity",
  "subtype": "species",
  "title": "Cardeal-amarelo",
  "subject": {
    "canonical_name": "Cardeal-amarelo",
    "scientific_name": "Gubernatrix cristata",
    "type_label": "Ave campestre ameaçada"
  },
  "themes": ["pampa", "aves", "fauna", "biodiversidade", "conservacao"],
  "canonical_facts": [
    {
      "fact_id": "fact_cardeal_habitat",
      "statement": "Habita áreas abertas de espinilho e campos com vegetação arbustiva no sul do Brasil.",
      "source_ids": ["source_icmbio_2018"],
      "confidence": "verified"
    }
  ],
  "modalities": {
    "image": ["assets/images/cardeal_amarelo.png"],
    "audio": ["assets/audio/cardeal_canto.ogg"]
  },
  "supported_roles": ["attract", "engage", "ambient"],
  "provenance": { "reviewed": true }
})";
    }

    // 2. species_capivara_002
    {
        std::ofstream f(dir / "catalog" / "atoms" / "species_capivara_002.json");
        f << R"({
  "schema_version": "0.1",
  "content_id": "species_capivara_002",
  "type": "entity",
  "subtype": "species",
  "title": "Capivara",
  "subject": {
    "canonical_name": "Capivara",
    "scientific_name": "Hydrochoerus hydrochaeris",
    "type_label": "Maior roedor do mundo"
  },
  "themes": ["pampa", "fauna", "banhados"],
  "canonical_facts": [
    {
      "fact_id": "fact_capivara_semi",
      "statement": "Mamífero semi-aquático associado a corpos d'água.",
      "source_ids": ["source_ufrgs_2023"],
      "confidence": "verified"
    }
  ],
  "modalities": {
    "image": ["assets/images/capivara.png"]
  },
  "supported_roles": ["attract", "engage", "ambient"],
  "provenance": { "reviewed": true }
})";
    }

    // 3. place_campos_sulinos_001
    {
        std::ofstream f(dir / "catalog" / "atoms" / "place_campos_sulinos_001.json");
        f << R"({
  "schema_version": "0.1",
  "content_id": "place_campos_sulinos_001",
  "type": "place",
  "subtype": "biome",
  "title": "Campos Sulinos",
  "subject": {
    "canonical_name": "Campos Sulinos",
    "type_label": "Ecossistema campestre do Pampa"
  },
  "themes": ["pampa", "vegetacao", "conservacao"],
  "canonical_facts": [
    {
      "fact_id": "fact_campos_gramineas",
      "statement": "Dominados por centenas de espécies de gramíneas nativas.",
      "source_ids": ["source_ufrgs_2023"],
      "confidence": "verified"
    }
  ],
  "modalities": {
    "image": ["assets/images/campos_sulinos.png"]
  },
  "supported_roles": ["ambient", "deepen"],
  "provenance": { "reviewed": true }
})";
    }

    // 4. phenomenon_pastejo_001
    {
        std::ofstream f(dir / "catalog" / "atoms" / "phenomenon_pastejo_001.json");
        f << R"({
  "schema_version": "0.1",
  "content_id": "phenomenon_pastejo_001",
  "type": "concept",
  "subtype": "process",
  "title": "Pastejo Tradicional",
  "subject": {
    "canonical_name": "Pastejo Tradicional",
    "type_label": "Processo ecossistêmico coevolutivo"
  },
  "themes": ["pampa", "pecuaria_sustentavel", "conservacao"],
  "canonical_facts": [
    {
      "fact_id": "fact_pastejo_manutencao",
      "statement": "O pastejo moderado impede o adensamento arbustivo homogêneo.",
      "source_ids": ["source_ufrgs_2023"],
      "confidence": "verified"
    }
  ],
  "modalities": {
    "image": ["assets/images/pastejo.png"]
  },
  "supported_roles": ["deepen", "engage"],
  "provenance": { "reviewed": true }
})";
    }

    // relations.json
    {
        std::ofstream f(dir / "catalog" / "relations" / "relations.json");
        f << R"([
  {
    "relation_id": "rel_cardeal_habitat_campos",
    "type": "habitat_of",
    "from": "place_campos_sulinos_001",
    "to": "species_cardeal_001",
    "strength": 0.9,
    "description": "Campos com espinilho são o habitat primordial do cardeal-amarelo."
  },
  {
    "relation_id": "rel_pastejo_campos",
    "type": "maintains",
    "from": "phenomenon_pastejo_001",
    "to": "place_campos_sulinos_001",
    "strength": 0.85,
    "description": "O pastejo tradicional preserva a diversidade florística campestre."
  }
])";
    }

    // recipes.json
    {
        std::ofstream f(dir / "catalog" / "recipes" / "recipes.json");
        f << R"([
  {
    "recipe_id": "discover_by_sound",
    "name": "Descobrir pelo Canto",
    "description": "Apresenta o som característico de uma espécie para aguçar a curiosidade e revelar sua identidade e importância ecológica.",
    "requires": ["subject.audio", "subject.image", "subject.name"],
    "steps": [
      "play_audio",
      "ask_identification",
      "reveal_image",
      "reveal_name",
      "show_micro_fact",
      "offer_deepen"
    ]
  },
  {
    "recipe_id": "discover_by_image",
    "name": "Descobrir pela Imagem",
    "description": "Revela um detalhe ou paisagem e convida a reconhecer e aprofundar.",
    "requires": ["subject.image", "subject.name"],
    "steps": [
      "reveal_image",
      "reveal_name",
      "show_micro_fact",
      "offer_deepen"
    ]
  },
  {
    "recipe_id": "contemplate",
    "name": "Contemplação do Pampa",
    "description": "Cena ambiental contemplativa sem exigência de resposta.",
    "requires": ["subject.image"],
    "steps": [
      "reveal_image",
      "show_micro_fact"
    ]
  }
])";
    }

    // variants.json
    {
        std::ofstream f(dir / "catalog" / "variants" / "variants.json");
        f << R"([
  {
    "variant_id": "var_cardeal_ambient_01",
    "content_id": "species_cardeal_001",
    "role": "ambient",
    "audience": "general",
    "interaction": "none",
    "duration_hint_seconds": 8,
    "presentation": {
      "title": "Cardeal-amarelo",
      "text": "A presença viva e rara que habita os campos do Pampa.",
      "media": ["assets/images/cardeal_amarelo.png"]
    }
  },
  {
    "variant_id": "var_cardeal_attract_01",
    "content_id": "species_cardeal_001",
    "role": "attract",
    "audience": "general",
    "interaction": "explore",
    "duration_hint_seconds": 10,
    "presentation": {
      "title": "Cardeal-amarelo",
      "text": "Conheça a ave rara dos espinilhos e gramíneas nativas do Pampa gaúcho.",
      "media": ["assets/images/cardeal_amarelo.png"],
      "options": ["Cardeal-amarelo", "Campos Sulinos", "Pastejo Tradicional"]
    }
  },
  {
    "variant_id": "var_cardeal_engage_01",
    "content_id": "species_cardeal_001",
    "role": "engage",
    "audience": "general",
    "interaction": "explore",
    "duration_hint_seconds": 12,
    "presentation": {
      "title": "Vida no Pampa",
      "text": "Uma joia alada ameaçada de extinção que depende do cuidado com o campo nativo.",
      "media": ["assets/images/cardeal_amarelo.png"],
      "options": ["Cardeal-amarelo", "Teia Ecológica", "Próxima Descoberta"]
    }
  },
  {
    "variant_id": "var_campos_ambient_01",
    "content_id": "place_campos_sulinos_001",
    "role": "ambient",
    "audience": "general",
    "interaction": "none",
    "duration_hint_seconds": 8,
    "presentation": {
      "title": "Campos Sulinos",
      "text": "Mais de 3.000 espécies vegetais sob o horizonte aberto do Pampa.",
      "media": ["assets/images/campos_sulinos.png"]
    }
  }
])";
    }

    return dir;
}

} // namespace elo::test
