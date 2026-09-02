return {
  {
    "nvim-treesitter/nvim-treesitter",
    opts = {
      ensure_installed = {
        "lua",
        "python",
        "javascript",
        "typescript",
        "c",
        "cpp",
        "go",
        "markdown",
        "markdown_inline",
        "bash",
        "vim",
        "vimdoc",
      },
      highlight = { enable = true },
      indent = { enable = false }, -- Explicitly disable Treesitter indentation
    },
  },

  -- Install Mason
  {
    "williamboman/mason.nvim",
  },

  -- Install Mason LSP bridge
  {
    "williamboman/mason-lspconfig.nvim",
  },

  -- Install Neovim native LSP config
  {
    "neovim/nvim-lspconfig",
    config = function()
      require("configs.lspconfig")
    end,
  },

  {
    "stevearc/conform.nvim",
    event = "BufWritePre",
    config = function()
      require("configs.conform").setup()
    end,
  },

  -- Completion plugins
  {
    "hrsh7th/nvim-cmp",
    config = function()
      require("configs.cmp")
    end,
  },
  { "hrsh7th/cmp-nvim-lsp" },
  { "hrsh7th/cmp-buffer" },
  { "hrsh7th/cmp-path" },
  { "L3MON4D3/LuaSnip" },
  { "saadparwaiz1/cmp_luasnip" },
}
