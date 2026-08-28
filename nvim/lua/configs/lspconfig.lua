local nvlsp = require "nvchad.configs.lspconfig"

-- Initialize NvChad's default LSP configurations (modern way for Neovim 0.11+)
-- This sets up the LspAttach autocommand, global capabilities, and lua_ls.
if nvlsp.defaults then
  nvlsp.defaults()
end

local on_init = nvlsp.on_init
local capabilities = nvlsp.capabilities

require("mason").setup()

require("mason-lspconfig").setup {
  ensure_installed = { "pylsp", "clangd", "gopls", "ts_ls", "html", "cssls", "emmet_ls", "eslint" },
}

local servers = { "pylsp", "clangd", "gopls", "ts_ls", "html", "cssls", "emmet_ls", "eslint" }

-- Configure and enable additional servers
for _, lsp in ipairs(servers) do
  local opts = {
    on_init = on_init,
    capabilities = capabilities,
  }

  if lsp == "clangd" then
    opts.cmd = {
      "clangd",
      "--offset-encoding=utf-16",
    }
  end

  if lsp == "pylsp" then
    opts.handlers = {
      ["textDocument/publishDiagnostics"] = function(err, result, ctx, config)
        result.diagnostics = vim.tbl_filter(function(d)
          -- Filter out specific pylsp warnings/errors
          return not vim.tbl_contains({ "E225", "W291", "E501", "E231", "E302", "W293" }, d.code)
        end, result.diagnostics)
        vim.lsp.handlers["textDocument/publishDiagnostics"](err, result, ctx, config)
      end,
    }
    opts.settings = {
      pylsp = {
        plugins = {
          black = { enabled = false },
          autopep8 = { enabled = true },
          yapf = { enabled = false },
          pylint = { enabled = false },
          flake8 = { enabled = false },
          pycodestyle = { enabled = false },
          mccabe = { enabled = false },
          pyflakes = { enabled = true },
          pylsp_mypy = { enabled = true, live_mode = false },
          rope_completion = { enabled = false },
          rope_autoimport = { enabled = false },
        },
      },
    }
  end

  -- Apply server-specific configuration and enable
  vim.lsp.config(lsp, opts)
  vim.lsp.enable(lsp)
end

-- Configure diagnostics visuals
vim.diagnostic.config({
  virtual_text = {
    prefix = "●",
  },
  signs = true,
  underline = true,
  update_in_insert = false,
  severity_sort = true,
})
